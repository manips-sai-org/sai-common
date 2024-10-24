/**
 * RedisClient.cpp
 *
 * Author: Toki Migimatsu
 *         Shameek Ganguly
 * Created: April 2017
 */

#include "RedisClient.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace Sai2Common {

RedisClient::RedisClient(const std::string& key_namespace_prefix) {
		if(!key_namespace_prefix.empty()) {
			_prefix = key_namespace_prefix + "::";
		}
	}

void RedisClient::connect(const std::string& hostname, const int port,
						  const struct timeval& timeout) {
	// Connect to new server
	_context.reset(nullptr);
	redisContext* c = redisConnectWithTimeout(hostname.c_str(), port, timeout);
	std::unique_ptr<redisContext, redisContextDeleter> context(c);

	// Check for errors
	if (!context)
		throw std::runtime_error(
			"RedisClient: Could not allocate redis context.");
	if (context->err)
		throw std::runtime_error(
			"RedisClient: Could not connect to redis server: " +
			std::string(context->errstr));

	// Save context
	_context = std::move(context);

	// create default send and receive groups
	createNewSendGroup("default");
	createNewReceiveGroup("default");
}

std::unique_ptr<redisReply, redisReplyDeleter> RedisClient::command(
	const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	redisReply* reply = (redisReply*)redisvCommand(_context.get(), format, ap);
	va_end(ap);
	return std::unique_ptr<redisReply, redisReplyDeleter>(reply);
}

void RedisClient::ping() {
	auto reply = command("PING");
	std::cout << std::endl
			  << "RedisClient: PING " << _context->tcp.host << ":"
			  << _context->tcp.port << std::endl;
	if (!reply) throw std::runtime_error("RedisClient: PING failed.");
	std::cout << "Reply: " << reply->str << std::endl << std::endl;
}

std::string RedisClient::get(const std::string& key) {
	const std::string key_with_prefix = _prefix + key;
	// Call GET command
	auto reply = command("GET %s", key_with_prefix.c_str());

	// Check for errors
	if (!reply || reply->type == REDIS_REPLY_ERROR ||
		reply->type == REDIS_REPLY_NIL)
		throw std::runtime_error("RedisClient: GET '" + key_with_prefix + "' failed.");
	if (reply->type != REDIS_REPLY_STRING)
		throw std::runtime_error("RedisClient: GET '" + key_with_prefix +
								 "' returned non-string value.");

	// Return value
	return reply->str;
}

void RedisClient::set(const std::string& key, const std::string& value) {
	const std::string key_with_prefix = _prefix + key;
	// Call SET command
	auto reply = command("SET %s %s", key_with_prefix.c_str(), value.c_str());

	// Check for errors
	if (!reply || reply->type == REDIS_REPLY_ERROR)
		throw std::runtime_error("RedisClient: SET '" + key_with_prefix + "' '" + value +
								 "' failed.");
}

void RedisClient::del(const std::string& key) {
	const std::string key_with_prefix = _prefix + key;
	// Call DEL command
	auto reply = command("DEL %s", key_with_prefix.c_str());

	// Check for errors
	if (!reply || reply->type == REDIS_REPLY_ERROR)
		throw std::runtime_error("RedisClient: DEL '" + key_with_prefix + "' failed.");
}

bool RedisClient::exists(const std::string& key) {
	const std::string key_with_prefix = _prefix + key;
	// Call GET command
	auto reply = command("EXISTS %s", key_with_prefix.c_str());

	// Check for errors
	if (!reply || reply->type == REDIS_REPLY_ERROR ||
		reply->type == REDIS_REPLY_NIL)
		throw std::runtime_error("RedisClient: EXISTS '" + key_with_prefix + "' failed.");
	if (reply->type != REDIS_REPLY_INTEGER)
		throw std::runtime_error("RedisClient: EXISTS '" + key_with_prefix +
								 "' returned non-integer value.");

	bool return_value = (reply->integer == 1);

	if (!return_value && (reply->integer != 0)) {
		throw std::runtime_error("RedisClient: EXISTS '" + key_with_prefix +
								 "' returned unexpected value (not 0 or 1)");
	}

	return return_value;
}

std::vector<std::string> RedisClient::pipeget(
	const std::vector<std::string>& keys) {
	// Prepare key list
	for (const auto& key : keys) {
		const std::string key_with_prefix = _prefix + key;
		redisAppendCommand(_context.get(), "GET %s", key_with_prefix.c_str());
	}

	// Collect values
	std::vector<std::string> values;
	for (const auto& key : keys) {
		const std::string key_with_prefix = _prefix + key;
		redisReply* r;
		if (redisGetReply(_context.get(), (void**)&r) == REDIS_ERR)
			throw std::runtime_error(
				"RedisClient: Pipeline GET command failed for key: " + key_with_prefix + ".");

		std::unique_ptr<redisReply, redisReplyDeleter> reply(r);
		if (reply->type != REDIS_REPLY_STRING)
			throw std::runtime_error(
				"RedisClient: Pipeline GET command returned non-string value for key: " +
				key_with_prefix + ".");

		values.push_back(reply->str);
	}

	return values;
}

void RedisClient::pipeset(
	const std::vector<std::pair<std::string, std::string>>& keyvals) {
	// Prepare key list
	for (const auto& keyval : keyvals) {
		const std::string key_with_prefix = _prefix + keyval.first;
		redisAppendCommand(_context.get(), "SET %s %s", key_with_prefix.c_str(),
						   keyval.second.c_str());
	}

	for (const auto& keyval : keyvals) {
		const std::string key_with_prefix = _prefix + keyval.first;
		redisReply* r;
		if (redisGetReply(_context.get(), (void**)&r) == REDIS_ERR)
			throw std::runtime_error(
				"RedisClient: Pipeline SET command failed for key: " + key_with_prefix + ".");

		std::unique_ptr<redisReply, redisReplyDeleter> reply(r);
		if (reply->type == REDIS_REPLY_ERROR)
			throw std::runtime_error(
				"RedisClient: Pipeline SET command failed for key: " + key_with_prefix + ".");
	}
}

std::vector<std::string> RedisClient::mget(
	const std::vector<std::string>& keys) {
	// Prepare key list
	std::vector<const char*> argv = {"MGET"};
	std::vector<const std::string> prefixed_keys;
	for (const auto& key : keys) {
		prefixed_keys.push_back(_prefix + key);
	}
	for (const auto& key : prefixed_keys) {
		argv.push_back(key.c_str());
	}

	// Call MGET command with variable argument formatting
	redisReply* r = (redisReply*)redisCommandArgv(_context.get(), argv.size(),
												  &argv[0], nullptr);
	std::unique_ptr<redisReply, redisReplyDeleter> reply(r);

	// Check for errors
	if (!reply || reply->type != REDIS_REPLY_ARRAY)
		throw std::runtime_error("RedisClient: MGET command failed.");

	// Collect values
	std::vector<std::string> values;
	for (size_t i = 0; i < reply->elements; i++) {
		if (reply->element[i]->type != REDIS_REPLY_STRING)
			throw std::runtime_error(
				"RedisClient: MGET command returned non-string values.");

		values.push_back(reply->element[i]->str);
	}
	return values;
}

void RedisClient::mset(
	const std::vector<std::pair<std::string, std::string>>& keyvals) {
	// Prepare key-value list
	std::vector<const char*> argv = {"MSET"};
	std::vector<std::string> prefixed_keys;
	for (const auto& keyval : keyvals) {
		prefixed_keys.push_back(_prefix + keyval.first);
	}
	for (size_t i = 0; i < keyvals.size(); i++) {
		argv.push_back(prefixed_keys.at(i).c_str());
		argv.push_back(keyvals.at(i).second.c_str());
	}

	// Call MSET command with variable argument formatting
	redisReply* r = (redisReply*)redisCommandArgv(_context.get(), argv.size(),
												  &argv[0], nullptr);
	std::unique_ptr<redisReply, redisReplyDeleter> reply(r);

	// Check for errors
	if (!reply || reply->type == REDIS_REPLY_ERROR)
		throw std::runtime_error("RedisClient: MSET command failed.");
}

void RedisClient::createNewReceiveGroup(const std::string& group_name) {
	if (receiveGroupExists(group_name)) {
		cout << "receive group already exists with this name. Not creating a "
				"new one"
			 << endl;
		return;
	}

	_receive_group_names.push_back(group_name);
	_keys_to_receive[group_name] = vector<string>();
	_objects_to_receive[group_name] = vector<void*>();
	_objects_to_receive_types[group_name] = vector<RedisSupportedTypes>();
}

void RedisClient::createNewSendGroup(const std::string& group_name) {
	if (sendGroupExists(group_name)) {
		cout << "send group already exists with this name. Not creating a new "
				"one"
			 << endl;
		return;
	}

	_send_group_names.push_back(group_name);
	_keys_to_send[group_name] = vector<string>();
	_objects_to_send[group_name] = vector<const void*>();
	_objects_to_send_types[group_name] = vector<RedisSupportedTypes>();
	_objects_to_send_sizes[group_name] = vector<pair<int, int>>();
}

void RedisClient::deleteSendGroup(const std::string& group_name) {
	if (!sendGroupExists(group_name)) {
		cout << "send group does not exist with this name " << group_name
			 << ". Cannot delete it" << endl;
		return;
	}

	_send_group_names.erase(std::remove(_send_group_names.begin(),
										_send_group_names.end(), group_name),
							_send_group_names.end());
	_keys_to_send.erase(group_name);
	_objects_to_send.erase(group_name);
	_objects_to_send_types.erase(group_name);
	_objects_to_send_sizes.erase(group_name);
}

void RedisClient::deleteReceiveGroup(const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		cout << "receive group does not exist with this name " << group_name
			 << ". Cannot delete it" << endl;
		return;
	}

	_receive_group_names.erase(
		std::remove(_receive_group_names.begin(), _receive_group_names.end(),
					group_name),
		_receive_group_names.end());
	_keys_to_receive.erase(group_name);
	_objects_to_receive.erase(group_name);
	_objects_to_receive_types.erase(group_name);
}

void RedisClient::addToReceiveGroup(const std::string& key, double& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to "
								 "receive");
	}

	setDouble(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(DOUBLE_NUMBER);
}

void RedisClient::addToReceiveGroup(const std::string& key, std::string& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to "
								 "receive");
	}

	set(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(STRING);
}

void RedisClient::addToReceiveGroup(const std::string& key, int& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to "
								 "receive");
	}

	setInt(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(INT_NUMBER);
}

void RedisClient::addToReceiveGroup(const std::string& key, bool& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to "
								 "receive");
	}

	setBool(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(BOOL);
}

void RedisClient::addToSendGroup(const std::string& key, const double& object,
								 const std::string& group_name) {
	if (!sendGroupExists(group_name)) {
		throw std::runtime_error("Send group with name [" + group_name +
								 "] not found, cannot add object to send");
	}

	_keys_to_send[group_name].push_back(key);
	_objects_to_send[group_name].push_back(&object);
	_objects_to_send_types[group_name].push_back(DOUBLE_NUMBER);
	_objects_to_send_sizes[group_name].push_back(std::make_pair(0, 0));
}

void RedisClient::addToSendGroup(const std::string& key,
								 const std::string& object,
								 const std::string& group_name) {
	if (!sendGroupExists(group_name)) {
		throw std::runtime_error("Send group with name [" + group_name +
								 "] not found, cannot add object to send");
	}

	_keys_to_send[group_name].push_back(key);
	_objects_to_send[group_name].push_back(&object);
	_objects_to_send_types[group_name].push_back(STRING);
	_objects_to_send_sizes[group_name].push_back(std::make_pair(0, 0));
}

void RedisClient::addToSendGroup(const std::string& key, const int& object,
								 const std::string& group_name) {
	if (!sendGroupExists(group_name)) {
		throw std::runtime_error("Send group with name [" + group_name +
								 "] not found, cannot add object to send");
	}

	_keys_to_send[group_name].push_back(key);
	_objects_to_send[group_name].push_back(&object);
	_objects_to_send_types[group_name].push_back(INT_NUMBER);
	_objects_to_send_sizes[group_name].push_back(std::make_pair(0, 0));
}

void RedisClient::addToSendGroup(const std::string& key, const bool& object,
								 const std::string& group_name) {
	if (!sendGroupExists(group_name)) {
		throw std::runtime_error("Send group with name [" + group_name +
								 "] not found, cannot add object to send");
	}

	_keys_to_send[group_name].push_back(key);
	_objects_to_send[group_name].push_back(&object);
	_objects_to_send_types[group_name].push_back(BOOL);
	_objects_to_send_sizes[group_name].push_back(std::make_pair(0, 0));
}

void RedisClient::receiveAllFromGroup(const std::string& group_name) {
	std::vector<std::string> group_names = {group_name};
	receiveAllFromGroup(group_names);
}

void RedisClient::receiveAllFromGroup(
	const std::vector<std::string>& group_names) {
	for (const auto& group_name : group_names) {
		if (!receiveGroupExists(group_name)) {
			throw std::runtime_error("Receive group with name [" + group_name +
									 "] not found, cannot "
									 "receiveAllFromGroup");
		}
	}

	std::vector<std::string> keys_to_receive;
	for (const auto& group_name : group_names) {
		keys_to_receive.insert(keys_to_receive.end(),
							   _keys_to_receive.at(group_name).begin(),
							   _keys_to_receive.at(group_name).end());
	}

	std::vector<std::string> return_values = mget(keys_to_receive);
	int return_values_index = 0;

	for (const auto& group_name : group_names) {
		for (int i = 0; i < _objects_to_receive.at(group_name).size(); ++i) {
			switch (_objects_to_receive_types.at(group_name).at(i)) {
				case DOUBLE_NUMBER: {
					double* tmp_pointer =
						(double*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer = stod(return_values[return_values_index]);
				} break;

				case INT_NUMBER: {
					int* tmp_pointer =
						(int*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer = stoi(return_values[return_values_index]);
				} break;

				case BOOL: {
					bool* tmp_pointer =
						(bool*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer =
						(bool)stoi(return_values[return_values_index]);
				} break;

				case STRING: {
					std::string* tmp_pointer =
						(std::string*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer = return_values[return_values_index];
				} break;

				case EIGEN_OBJECT: {
					double* tmp_pointer =
						(double*)_objects_to_receive.at(group_name).at(i);

					Eigen::MatrixXd tmp_return_matrix =
						RedisClient::decodeEigenMatrix(
							return_values[return_values_index]);

					int nrows = tmp_return_matrix.rows();
					int ncols = tmp_return_matrix.cols();

					for (int k = 0; k < nrows; k++) {
						for (int l = 0; l < ncols; l++) {
							tmp_pointer[k + ncols * l] =
								tmp_return_matrix(k, l);
						}
					}
				} break;

				default:
					throw std::runtime_error(
						"RedisClient: Unknown type in "
						"receiveAllFromGroup");
					break;
			}
			return_values_index++;
		}
	}
}

void RedisClient::sendAllFromGroup(const std::string& group_name) {
	std::vector<std::string> group_names = {group_name};
	sendAllFromGroup(group_names);
}

void RedisClient::sendAllFromGroup(
	const std::vector<std::string>& group_names) {
	for (const auto& group_name : group_names) {
		if (!sendGroupExists(group_name)) {
			throw std::runtime_error("Send group with name [" + group_name +
									 "] not found, cannot sendAllFromGroup");
		}
	}

	std::vector<std::pair<std::string, std::string>> write_key_value_pairs;

	for (const auto& group_name : group_names) {
		for (int i = 0; i < _keys_to_send.at(group_name).size(); i++) {
			std::string encoded_value = "";

			switch (_objects_to_send_types.at(group_name).at(i)) {
				case DOUBLE_NUMBER: {
					double* tmp_pointer =
						(double*)_objects_to_send.at(group_name).at(i);
					encoded_value = std::to_string(*tmp_pointer);
				} break;

				case INT_NUMBER: {
					int* tmp_pointer =
						(int*)_objects_to_send.at(group_name).at(i);
					encoded_value = std::to_string(*tmp_pointer);
				} break;

				case BOOL: {
					bool* tmp_pointer =
						(bool*)_objects_to_send.at(group_name).at(i);
					encoded_value = *tmp_pointer ? "1" : "0";
				} break;

				case STRING: {
					std::string* tmp_pointer =
						(std::string*)_objects_to_send.at(group_name).at(i);
					encoded_value = (*tmp_pointer);
				} break;

				case EIGEN_OBJECT: {
					double* tmp_pointer =
						(double*)_objects_to_send.at(group_name).at(i);
					int nrows =
						_objects_to_send_sizes.at(group_name).at(i).first;
					int ncols =
						_objects_to_send_sizes.at(group_name).at(i).second;

					Eigen::MatrixXd tmp_matrix =
						Eigen::MatrixXd::Zero(nrows, ncols);
					for (int k = 0; k < nrows; k++) {
						for (int l = 0; l < ncols; l++) {
							tmp_matrix(k, l) = tmp_pointer[k + ncols * l];
						}
					}

					encoded_value = encodeEigenMatrix(tmp_matrix);
				} break;
			}

			if (encoded_value != "") {
				write_key_value_pairs.push_back(make_pair(
					_keys_to_send.at(group_name).at(i), encoded_value));
			}
		}
	}

	mset(write_key_value_pairs);
}

bool RedisClient::sendGroupExists(const std::string& group_name) const {
	auto it = std::find(_send_group_names.begin(), _send_group_names.end(),
						group_name);
	return it != _send_group_names.end();
}

bool RedisClient::receiveGroupExists(const std::string& group_name) const {
	auto it = std::find(_receive_group_names.begin(),
						_receive_group_names.end(), group_name);
	return it != _receive_group_names.end();
}

static inline Eigen::MatrixXd decodeEigenMatrixWithDelimiters(
	const std::string& str, char col_delimiter, char row_delimiter,
	const std::string& delimiter_set, size_t idx_row_end = std::string::npos) {
	// Count number of columns
	size_t num_cols = 0;
	size_t idx = 0;
	size_t idx_col_end = str.find_first_of(row_delimiter);
	while (idx < idx_col_end) {
		// Skip over extra whitespace
		idx = str.find_first_not_of(' ', idx);
		if (idx >= idx_col_end) break;

		// Find next delimiter
		idx = str.find_first_of(col_delimiter, idx + 1);
		++num_cols;
	}
	if (idx > idx_col_end) idx = idx_col_end;

	// Count number of rows
	size_t num_rows = 1;  // First row already traversed
	while (idx < idx_row_end) {
		// Skip over irrelevant characters
		idx = str.find_first_not_of(row_delimiter, idx);
		if (idx >= idx_row_end) break;

		// Find next delimiter
		idx = str.find_first_of(row_delimiter, idx + 1);
		++num_rows;
	}

	// Check number of rows and columns
	if (num_cols == 0)
		throw std::runtime_error(
			"RedisClient: Failed to decode Eigen Matrix from: " + str + ".");
	if (num_rows == 1) {
		// Convert to vector
		num_rows = num_cols;
		num_cols = 1;
	}

	// Parse matrix
	Eigen::MatrixXd matrix(num_rows, num_cols);
	std::string str_local(str);
	for (char delimiter : delimiter_set) {
		std::replace(str_local.begin(), str_local.end(), delimiter, ' ');
	}
	std::stringstream ss(str_local);
	for (size_t i = 0; i < num_rows; ++i) {
		for (size_t j = 0; j < num_cols; ++j) {
			std::string val;
			ss >> val;
			try {
				matrix(i, j) = std::stod(val);
			} catch (const std::exception& e) {
				throw std::runtime_error(
					"RedisClient: Failed to decode Eigen Matrix from: " + str +
					".");
			}
		}
	}

	return matrix;
}

Eigen::MatrixXd RedisClient::decodeEigenMatrix(const std::string& str) {
	// Find last nested row delimiter
	size_t idx_row_end = str.find_last_of(']');
	if (idx_row_end != std::string::npos) {
		size_t idx_temp = str.substr(0, idx_row_end).find_last_of(']');
		if (idx_temp != std::string::npos) idx_row_end = idx_temp;
	}
	return decodeEigenMatrixWithDelimiters(str, ',', ']', ",[]", idx_row_end);
}

}  // namespace Sai2Common
