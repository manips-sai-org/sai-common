/**
 * ZmqClient.cpp
 *
 * Adapted from RedisClient
 */

#include "ZmqClient.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace SaiCommon {

ZmqClient::ZmqClient(const std::string& key_namespace_prefix) {
	if(!key_namespace_prefix.empty()) {
		_prefix = key_namespace_prefix + "::";
	}
}

void ZmqClient::connect(const std::string& hostname, const int port,
						  const struct timeval& timeout) {
	_context = std::make_unique<zmq::context_t>(1);
	_socket = std::make_unique<zmq::socket_t>(*_context, zmq::socket_type::req);
	
	// Convert timeval to milliseconds for ZMQ options
	int timeout_ms = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
	_socket->set(zmq::sockopt::rcvtimeo, timeout_ms);
	_socket->set(zmq::sockopt::sndtimeo, timeout_ms);

	_host = hostname;
	_port = port;
	std::string address = "tcp://" + hostname + ":" + std::to_string(port);
	_socket->connect(address);

	createNewSendGroup("default");
	createNewReceiveGroup("default");
}

std::vector<std::string> ZmqClient::request(const std::vector<std::string>& args) {
	// Send multi-part message
	for (size_t i = 0; i < args.size(); ++i) {
		zmq::message_t msg(args[i].data(), args[i].size());
		zmq::send_flags flags = (i == args.size() - 1) ? zmq::send_flags::none : zmq::send_flags::sndmore;
		
		if (!_socket->send(msg, flags)) {
			throw std::runtime_error("ZmqClient: Send failed.");
		}
	}

	// Receive multi-part reply
	std::vector<std::string> replies;
	while (true) {
		zmq::message_t msg;
		if (!_socket->recv(msg, zmq::recv_flags::none)) {
			throw std::runtime_error("ZmqClient: Recv failed (timeout?).");
		}
		replies.push_back(std::string(static_cast<char*>(msg.data()), msg.size()));
		if (!msg.more()) break;
	}

	return replies;
}

void ZmqClient::ping() {
	auto reply = request({"PING"});
	std::cout << std::endl
			  << "ZmqClient: PING " << _host << ":"
			  << _port << std::endl;
	if (reply.empty()) throw std::runtime_error("ZmqClient: PING failed.");
	std::cout << "Reply: " << reply[0] << std::endl << std::endl;
}

std::string ZmqClient::get(const std::string& key) {
	const std::string key_with_prefix = _prefix + key;
	auto reply = request({"GET", key_with_prefix});

	if (reply.empty() || reply[0] == "ERROR" || reply[0] == "NIL")
		throw std::runtime_error("ZmqClient: GET '" + key_with_prefix + "' failed.");

	return reply[0];
}

void ZmqClient::set(const std::string& key, const std::string& value) {
	const std::string key_with_prefix = _prefix + key;
	auto reply = request({"SET", key_with_prefix, value});

	if (reply.empty() || reply[0] == "ERROR")
		throw std::runtime_error("ZmqClient: SET '" + key_with_prefix + "' '" + value + "' failed.");
}

void ZmqClient::set(const std::string& key, const std::vector<unsigned char>& value) {
	std::string val_str(value.begin(), value.end());
	auto reply = request({"SET", key, val_str});

	if (reply.empty() || reply[0] == "ERROR")
		throw std::runtime_error("ZmqClient: SET '" + key + "' 'binary data' failed.");
}

void ZmqClient::del(const std::string& key) {
	const std::string key_with_prefix = _prefix + key;
	auto reply = request({"DEL", key_with_prefix});

	if (reply.empty() || reply[0] == "ERROR")
		throw std::runtime_error("ZmqClient: DEL '" + key_with_prefix + "' failed.");
}

bool ZmqClient::exists(const std::string& key) {
	const std::string key_with_prefix = _prefix + key;
	auto reply = request({"EXISTS", key_with_prefix});

	if (reply.empty() || reply[0] == "ERROR")
		throw std::runtime_error("ZmqClient: EXISTS '" + key_with_prefix + "' failed.");

	int val = std::stoi(reply[0]);
	if (val != 0 && val != 1) {
		throw std::runtime_error("ZmqClient: EXISTS '" + key_with_prefix +
								 "' returned unexpected value (not 0 or 1)");
	}

	return val == 1;
}

std::vector<std::string> ZmqClient::pipeget(const std::vector<std::string>& keys) {
	// ZeroMQ REQ-REP pattern expects lock-step send/receive.
	// Falling back to MGET for logical equivalence.
	return mget(keys);
}

void ZmqClient::pipeset(const std::vector<std::pair<std::string, std::string>>& keyvals) {
	mset(keyvals);
}

std::vector<std::string> ZmqClient::mget(const std::vector<std::string>& keys) {
	std::vector<std::string> req = {"MGET"};
	for (const auto& key : keys) {
		req.push_back(_prefix + key);
	}

	auto reply = request(req);

	if (reply.empty() || reply[0] == "ERROR")
		throw std::runtime_error("ZmqClient: MGET command failed.");

	// If server returns a single block of response instead of proper multi-part,
	// you may need to parse it. Assuming server complies with multi-part reply here:
	return reply;
}

void ZmqClient::mset(const std::vector<std::pair<std::string, std::string>>& keyvals) {
	std::vector<std::string> req = {"MSET"};
	for (const auto& keyval : keyvals) {
		req.push_back(_prefix + keyval.first);
		req.push_back(keyval.second);
	}

	auto reply = request(req);

	if (reply.empty() || reply[0] == "ERROR")
		throw std::runtime_error("ZmqClient: MSET command failed.");
}

void ZmqClient::createNewReceiveGroup(const std::string& group_name) {
	if (receiveGroupExists(group_name)) {
		cout << "receive group already exists with this name. Not creating a "
				"new one" << endl;
		return;
	}

	_receive_group_names.push_back(group_name);
	_keys_to_receive[group_name] = vector<string>();
	_objects_to_receive[group_name] = vector<void*>();
	_objects_to_receive_types[group_name] = vector<SupportedTypes>();
}

void ZmqClient::createNewSendGroup(const std::string& group_name) {
	if (sendGroupExists(group_name)) {
		cout << "send group already exists with this name. Not creating a new "
				"one" << endl;
		return;
	}

	_send_group_names.push_back(group_name);
	_keys_to_send[group_name] = vector<string>();
	_objects_to_send[group_name] = vector<const void*>();
	_objects_to_send_types[group_name] = vector<SupportedTypes>();
	_objects_to_send_sizes[group_name] = vector<pair<int, int>>();
}

void ZmqClient::deleteSendGroup(const std::string& group_name) {
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

void ZmqClient::deleteReceiveGroup(const std::string& group_name) {
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

void ZmqClient::addToReceiveGroup(const std::string& key, double& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to receive");
	}

	setDouble(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(DOUBLE_NUMBER);
}

void ZmqClient::addToReceiveGroup(const std::string& key, std::string& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to receive");
	}

	set(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(STRING);
}

void ZmqClient::addToReceiveGroup(const std::string& key, int& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to receive");
	}

	setInt(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(INT_NUMBER);
}

void ZmqClient::addToReceiveGroup(const std::string& key, bool& object,
									const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error("Receive group with name [" + group_name +
								 "] not found, cannot add object to receive");
	}

	setBool(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(&object);
	_objects_to_receive_types[group_name].push_back(BOOL);
}

void ZmqClient::addToSendGroup(const std::string& key, const double& object,
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

void ZmqClient::addToSendGroup(const std::string& key, const std::string& object,
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

void ZmqClient::addToSendGroup(const std::string& key, const int& object,
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

void ZmqClient::addToSendGroup(const std::string& key, const bool& object,
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

void ZmqClient::receiveAllFromGroup(const std::string& group_name) {
	std::vector<std::string> group_names = {group_name};
	receiveAllFromGroup(group_names);
}

void ZmqClient::receiveAllFromGroup(const std::vector<std::string>& group_names) {
	for (const auto& group_name : group_names) {
		if (!receiveGroupExists(group_name)) {
			throw std::runtime_error("Receive group with name [" + group_name +
									 "] not found, cannot receiveAllFromGroup");
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
		for (size_t i = 0; i < _objects_to_receive.at(group_name).size(); ++i) {
			switch (_objects_to_receive_types.at(group_name).at(i)) {
				case DOUBLE_NUMBER: {
					double* tmp_pointer = (double*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer = stod(return_values[return_values_index]);
				} break;

				case INT_NUMBER: {
					int* tmp_pointer = (int*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer = stoi(return_values[return_values_index]);
				} break;

				case BOOL: {
					bool* tmp_pointer = (bool*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer = (bool)stoi(return_values[return_values_index]);
				} break;

				case STRING: {
					std::string* tmp_pointer = (std::string*)_objects_to_receive.at(group_name).at(i);
					*tmp_pointer = return_values[return_values_index];
				} break;

				case EIGEN_OBJECT: {
					double* tmp_pointer = (double*)_objects_to_receive.at(group_name).at(i);

					Eigen::MatrixXd tmp_return_matrix = ZmqClient::decodeEigenMatrix(return_values[return_values_index]);

					int nrows = tmp_return_matrix.rows();
					int ncols = tmp_return_matrix.cols();

					for (int k = 0; k < nrows; k++) {
						for (int l = 0; l < ncols; l++) {
							tmp_pointer[k + ncols * l] = tmp_return_matrix(k, l);
						}
					}
				} break;

				default:
					throw std::runtime_error("ZmqClient: Unknown type in receiveAllFromGroup");
			}
			return_values_index++;
		}
	}
}

void ZmqClient::sendAllFromGroup(const std::string& group_name) {
	std::vector<std::string> group_names = {group_name};
	sendAllFromGroup(group_names);
}

void ZmqClient::sendAllFromGroup(const std::vector<std::string>& group_names) {
	for (const auto& group_name : group_names) {
		if (!sendGroupExists(group_name)) {
			throw std::runtime_error("Send group with name [" + group_name +
									 "] not found, cannot sendAllFromGroup");
		}
	}

	std::vector<std::pair<std::string, std::string>> write_key_value_pairs;

	for (const auto& group_name : group_names) {
		for (size_t i = 0; i < _keys_to_send.at(group_name).size(); i++) {
			std::string encoded_value = "";

			switch (_objects_to_send_types.at(group_name).at(i)) {
				case DOUBLE_NUMBER: {
					double* tmp_pointer = (double*)_objects_to_send.at(group_name).at(i);
					encoded_value = std::to_string(*tmp_pointer);
				} break;

				case INT_NUMBER: {
					int* tmp_pointer = (int*)_objects_to_send.at(group_name).at(i);
					encoded_value = std::to_string(*tmp_pointer);
				} break;

				case BOOL: {
					bool* tmp_pointer = (bool*)_objects_to_send.at(group_name).at(i);
					encoded_value = *tmp_pointer ? "1" : "0";
				} break;

				case STRING: {
					std::string* tmp_pointer = (std::string*)_objects_to_send.at(group_name).at(i);
					encoded_value = (*tmp_pointer);
				} break;

				case EIGEN_OBJECT: {
					double* tmp_pointer = (double*)_objects_to_send.at(group_name).at(i);
					int nrows = _objects_to_send_sizes.at(group_name).at(i).first;
					int ncols = _objects_to_send_sizes.at(group_name).at(i).second;

					Eigen::MatrixXd tmp_matrix = Eigen::MatrixXd::Zero(nrows, ncols);
					for (int k = 0; k < nrows; k++) {
						for (int l = 0; l < ncols; l++) {
							tmp_matrix(k, l) = tmp_pointer[k + ncols * l];
						}
					}

					encoded_value = encodeEigenMatrix(tmp_matrix);
				} break;
			}

			if (encoded_value != "") {
				write_key_value_pairs.push_back(make_pair(_keys_to_send.at(group_name).at(i), encoded_value));
			}
		}
	}

	mset(write_key_value_pairs);
}

bool ZmqClient::sendGroupExists(const std::string& group_name) const {
	auto it = std::find(_send_group_names.begin(), _send_group_names.end(), group_name);
	return it != _send_group_names.end();
}

bool ZmqClient::receiveGroupExists(const std::string& group_name) const {
	auto it = std::find(_receive_group_names.begin(), _receive_group_names.end(), group_name);
	return it != _receive_group_names.end();
}

static inline Eigen::MatrixXd decodeEigenMatrixWithDelimiters(
	const std::string& str, char col_delimiter, char row_delimiter,
	const std::string& delimiter_set, size_t idx_row_end = std::string::npos) {
	size_t num_cols = 0;
	size_t idx = 0;
	size_t idx_col_end = str.find_first_of(row_delimiter);
	while (idx < idx_col_end) {
		idx = str.find_first_not_of(' ', idx);
		if (idx >= idx_col_end) break;
		idx = str.find_first_of(col_delimiter, idx + 1);
		++num_cols;
	}
	if (idx > idx_col_end) idx = idx_col_end;

	size_t num_rows = 1;
	while (idx < idx_row_end) {
		idx = str.find_first_not_of(row_delimiter, idx);
		if (idx >= idx_row_end) break;
		idx = str.find_first_of(row_delimiter, idx + 1);
		++num_rows;
	}

	if (num_cols == 0)
		throw std::runtime_error("ZmqClient: Failed to decode Eigen Matrix from: " + str + ".");
	if (num_rows == 1) {
		num_rows = num_cols;
		num_cols = 1;
	}

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
				throw std::runtime_error("ZmqClient: Failed to decode Eigen Matrix from: " + str + ".");
			}
		}
	}

	return matrix;
}

Eigen::MatrixXd ZmqClient::decodeEigenMatrix(const std::string& str) {
	size_t idx_row_end = str.find_last_of(']');
	if (idx_row_end != std::string::npos) {
		size_t idx_temp = str.substr(0, idx_row_end).find_last_of(']');
		if (idx_temp != std::string::npos) idx_row_end = idx_temp;
	}
	return decodeEigenMatrixWithDelimiters(str, ',', ']', ",[]", idx_row_end);
}

}  // namespace SaiCommon