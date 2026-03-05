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

namespace SaiCommon {

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
	return std::string(reply->str, reply->len);
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

void RedisClient::set(const std::string& key, const std::vector<unsigned char>& value) {
	const std::string key_with_prefix = _prefix + key;
	// Call SET command
	auto reply = command("SET %s %b", key_with_prefix.c_str(), value.data(),
						 value.size());

	// Check for errors
	if (!reply || reply->type == REDIS_REPLY_ERROR)
		throw std::runtime_error("RedisClient: SET '" + key_with_prefix +
								 "' '" + "binary data" + "' failed.");
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

		values.emplace_back(reply->str, reply->len);
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
	if (keys.empty()) {
		return {};
	}

	// Prepare key list
	std::vector<const char*> argv;
	argv.reserve(1 + keys.size());
	argv.push_back("MGET");

	std::vector<std::string> prefixed_keys;
	prefixed_keys.reserve(keys.size());
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
	values.reserve(reply->elements);
	for (size_t i = 0; i < reply->elements; i++) {
		if (reply->element[i]->type != REDIS_REPLY_STRING)
			throw std::runtime_error(
				"RedisClient: MGET command returned non-string values.");

		values.emplace_back(reply->element[i]->str, reply->element[i]->len);
	}
	return values;
}

void RedisClient::mset(
	const std::vector<std::pair<std::string, std::string>>& keyvals) {
	if (keyvals.empty()) {
		return;
	}

	// Prepare key-value list
	std::vector<const char*> argv;
	std::vector<size_t> argvlen;
	argv.reserve(1 + 2 * keyvals.size());
	argvlen.reserve(1 + 2 * keyvals.size());
	argv.push_back("MSET");
	argvlen.push_back(4);

	std::vector<std::string> prefixed_keys;
	prefixed_keys.reserve(keyvals.size());
	for (const auto& keyval : keyvals) {
		prefixed_keys.push_back(_prefix + keyval.first);
	}
	for (size_t i = 0; i < keyvals.size(); i++) {
		argv.push_back(prefixed_keys.at(i).c_str());
		argvlen.push_back(prefixed_keys.at(i).size());
		argv.push_back(keyvals.at(i).second.c_str());
		argvlen.push_back(keyvals.at(i).second.size());
	}

	// Call MSET command with variable argument formatting
	redisReply* r = (redisReply*)redisCommandArgv(_context.get(), argv.size(),
												  &argv[0], &argvlen[0]);
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

	_keys_to_receive[group_name] = vector<string>();
	_objects_to_receive[group_name] = vector<void*>();
	_objects_to_receive_types[group_name] = vector<RedisSupportedTypes>();
	_objects_to_receive_sizes[group_name] = vector<pair<int, int>>();
}

void RedisClient::createNewSendGroup(const std::string& group_name) {
	if (sendGroupExists(group_name)) {
		cout << "send group already exists with this name. Not creating a new "
				"one"
			 << endl;
		return;
	}

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

	_keys_to_receive.erase(group_name);
	_objects_to_receive.erase(group_name);
	_objects_to_receive_types.erase(group_name);
	_objects_to_receive_sizes.erase(group_name);
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
	_objects_to_receive_sizes[group_name].push_back(std::make_pair(0, 0));
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
	_objects_to_receive_sizes[group_name].push_back(std::make_pair(0, 0));
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
	_objects_to_receive_sizes[group_name].push_back(std::make_pair(0, 0));
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
	_objects_to_receive_sizes[group_name].push_back(std::make_pair(0, 0));
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
	struct ReceiveGroupData {
		const std::vector<std::string>* keys;
		const std::vector<void*>* objects;
		const std::vector<RedisSupportedTypes>* types;
		const std::vector<std::pair<int, int>>* sizes;
	};

	std::vector<ReceiveGroupData> groups;
	groups.reserve(group_names.size());

	size_t total_keys = 0;
	for (const auto& group_name : group_names) {
		auto keys_it = _keys_to_receive.find(group_name);
		auto objects_it = _objects_to_receive.find(group_name);
		auto types_it = _objects_to_receive_types.find(group_name);
		auto sizes_it = _objects_to_receive_sizes.find(group_name);

		if (keys_it == _keys_to_receive.end() ||
			objects_it == _objects_to_receive.end() ||
			types_it == _objects_to_receive_types.end() ||
			sizes_it == _objects_to_receive_sizes.end()) {
			throw std::runtime_error("Receive group with name [" + group_name +
									 "] not found, cannot "
									 "receiveAllFromGroup");
		}

		total_keys += keys_it->second.size();
		groups.push_back({&keys_it->second, &objects_it->second, &types_it->second,
						  &sizes_it->second});
	}

	std::vector<std::string> keys_to_receive;
	keys_to_receive.reserve(total_keys);
	for (const auto& group : groups) {
		keys_to_receive.insert(keys_to_receive.end(), group.keys->begin(),
							   group.keys->end());
	}

	std::vector<std::string> return_values = mget(keys_to_receive);
	if (return_values.size() != total_keys) {
		throw std::runtime_error(
			"RedisClient: MGET returned unexpected number of values in "
			"receiveAllFromGroup");
	}

	size_t return_values_index = 0;
	for (const auto& group : groups) {
		const auto& objects = *group.objects;
		const auto& types = *group.types;
		const auto& sizes = *group.sizes;

		for (size_t i = 0; i < objects.size(); ++i) {
			switch (types[i]) {
				case DOUBLE_NUMBER: {
					double* tmp_pointer = (double*)objects[i];
					*tmp_pointer = stod(return_values[return_values_index]);
				} break;

				case INT_NUMBER: {
					int* tmp_pointer = (int*)objects[i];
					*tmp_pointer = stoi(return_values[return_values_index]);
				} break;

				case BOOL: {
					bool* tmp_pointer = (bool*)objects[i];
					*tmp_pointer = (bool)stoi(return_values[return_values_index]);
				} break;

				case STRING: {
					std::string* tmp_pointer = (std::string*)objects[i];
					*tmp_pointer = return_values[return_values_index];
				} break;

				case EIGEN_OBJECT: {
					double* tmp_pointer = (double*)objects[i];
					const int expected_nrows = sizes[i].first;
					const int expected_ncols = sizes[i].second;

					Eigen::MatrixXd tmp_return_matrix = RedisClient::decodeEigenValue(
						return_values[return_values_index]);

					int nrows = tmp_return_matrix.rows();
					int ncols = tmp_return_matrix.cols();
					if (expected_nrows > 0 && expected_ncols > 0 &&
						(nrows != expected_nrows || ncols != expected_ncols)) {
						throw std::runtime_error(
							"RedisClient: Eigen size mismatch in "
							"receiveAllFromGroup");
					}
					std::memcpy(tmp_pointer, tmp_return_matrix.data(),
								static_cast<size_t>(nrows) *
									static_cast<size_t>(ncols) * sizeof(double));
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
	struct SendGroupData {
		const std::vector<std::string>* keys;
		const std::vector<const void*>* objects;
		const std::vector<RedisSupportedTypes>* types;
		const std::vector<std::pair<int, int>>* sizes;
	};

	std::vector<SendGroupData> groups;
	groups.reserve(group_names.size());

	size_t total_keys = 0;
	for (const auto& group_name : group_names) {
		auto keys_it = _keys_to_send.find(group_name);
		auto objects_it = _objects_to_send.find(group_name);
		auto types_it = _objects_to_send_types.find(group_name);
		auto sizes_it = _objects_to_send_sizes.find(group_name);

		if (keys_it == _keys_to_send.end() ||
			objects_it == _objects_to_send.end() ||
			types_it == _objects_to_send_types.end() ||
			sizes_it == _objects_to_send_sizes.end()) {
			throw std::runtime_error("Send group with name [" + group_name +
									 "] not found, cannot sendAllFromGroup");
		}

		total_keys += keys_it->second.size();
		groups.push_back({&keys_it->second, &objects_it->second, &types_it->second,
						  &sizes_it->second});
	}

	std::vector<std::pair<std::string, std::string>> write_key_value_pairs;
	write_key_value_pairs.reserve(total_keys);

	for (const auto& group : groups) {
		const auto& keys = *group.keys;
		const auto& objects = *group.objects;
		const auto& types = *group.types;
		const auto& sizes = *group.sizes;

		for (size_t i = 0; i < keys.size(); i++) {
			std::string encoded_value;
			switch (types[i]) {
				case DOUBLE_NUMBER: {
					double* tmp_pointer = (double*)objects[i];
					encoded_value = std::to_string(*tmp_pointer);
				} break;

				case INT_NUMBER: {
					int* tmp_pointer = (int*)objects[i];
					encoded_value = std::to_string(*tmp_pointer);
				} break;

				case BOOL: {
					bool* tmp_pointer = (bool*)objects[i];
					encoded_value = *tmp_pointer ? "1" : "0";
				} break;

				case STRING: {
					std::string* tmp_pointer = (std::string*)objects[i];
					encoded_value = (*tmp_pointer);
				} break;

				case EIGEN_OBJECT: {
					double* tmp_pointer = (double*)objects[i];
					int nrows = sizes[i].first;
					int ncols = sizes[i].second;
					const auto encoded_value_binary =
						encodeEigenMatrixBinary(tmp_pointer, nrows, ncols);
					encoded_value.assign(
						reinterpret_cast<const char*>(encoded_value_binary.data()),
						encoded_value_binary.size());
				} break;

				default:
					throw std::runtime_error(
						"RedisClient: Unknown type in sendAllFromGroup");
					break;
			}

			write_key_value_pairs.emplace_back(keys[i], std::move(encoded_value));
		}
	}

	mset(write_key_value_pairs);
}

bool RedisClient::sendGroupExists(const std::string& group_name) const {
	return _keys_to_send.find(group_name) != _keys_to_send.end();
}

bool RedisClient::receiveGroupExists(const std::string& group_name) const {
	return _keys_to_receive.find(group_name) != _keys_to_receive.end();
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

Eigen::MatrixXd RedisClient::decodeEigenValue(const std::string& value) {
	static const unsigned char kMagic[8] = {'S', 'A', 'I', 'E',
											'I', 'G', '0', '1'};
	if (value.size() < 16) {
		return decodeEigenMatrix(value);
	}

	const unsigned char* ptr =
		reinterpret_cast<const unsigned char*>(value.data());
	if (std::memcmp(ptr, kMagic, 8) != 0) {
		return decodeEigenMatrix(value);
	}
	ptr += 8;

	const uint32_t rows = static_cast<uint32_t>(ptr[0]) |
						  (static_cast<uint32_t>(ptr[1]) << 8) |
						  (static_cast<uint32_t>(ptr[2]) << 16) |
						  (static_cast<uint32_t>(ptr[3]) << 24);
	ptr += 4;
	const uint32_t cols = static_cast<uint32_t>(ptr[0]) |
						  (static_cast<uint32_t>(ptr[1]) << 8) |
						  (static_cast<uint32_t>(ptr[2]) << 16) |
						  (static_cast<uint32_t>(ptr[3]) << 24);
	ptr += 4;

	const size_t data_size =
		static_cast<size_t>(rows) * static_cast<size_t>(cols) * sizeof(double);
	const size_t expected_size = 16 + data_size;
	if (value.size() != expected_size) {
		throw std::runtime_error(
			"RedisClient: Corrupted binary Eigen payload in decodeEigenValue.");
	}

	Eigen::MatrixXd matrix(rows, cols);
	for (uint32_t i = 0; i < rows; ++i) {
		for (uint32_t j = 0; j < cols; ++j) {
			double v;
			std::memcpy(&v, ptr, sizeof(double));
			ptr += sizeof(double);
			matrix(static_cast<int>(i), static_cast<int>(j)) = v;
		}
	}

	return matrix;
}

std::vector<unsigned char> RedisClient::encodeEigenMatrixBinary(
	const double* data, int rows, int cols) {
	static const unsigned char kMagic[8] = {'S', 'A', 'I', 'E',
											'I', 'G', '0', '1'};
	const uint32_t rows_u32 = static_cast<uint32_t>(rows);
	const uint32_t cols_u32 = static_cast<uint32_t>(cols);
	const size_t data_size =
		static_cast<size_t>(rows_u32) * static_cast<size_t>(cols_u32) *
		sizeof(double);

	std::vector<unsigned char> out(16 + data_size);
	unsigned char* ptr = out.data();
	std::memcpy(ptr, kMagic, 8);
	ptr += 8;

	ptr[0] = static_cast<unsigned char>(rows_u32 & 0xff);
	ptr[1] = static_cast<unsigned char>((rows_u32 >> 8) & 0xff);
	ptr[2] = static_cast<unsigned char>((rows_u32 >> 16) & 0xff);
	ptr[3] = static_cast<unsigned char>((rows_u32 >> 24) & 0xff);
	ptr += 4;

	ptr[0] = static_cast<unsigned char>(cols_u32 & 0xff);
	ptr[1] = static_cast<unsigned char>((cols_u32 >> 8) & 0xff);
	ptr[2] = static_cast<unsigned char>((cols_u32 >> 16) & 0xff);
	ptr[3] = static_cast<unsigned char>((cols_u32 >> 24) & 0xff);
	ptr += 4;

	std::memcpy(ptr, data, data_size);
	return out;
}

}  // namespace SaiCommon
