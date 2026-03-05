/**
 * ZmqClient.h
 *
 * Adapted from RedisClient
 */

#ifndef ZMQ_CLIENT_H
#define ZMQ_CLIENT_H

#include <zmq.hpp>

#include <Eigen/Core>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace SaiCommon {

/**
 * @brief A C++ wrapper for a ZeroMQ Request-Reply Key-Value store with
 * convenience functions for common KV commands and getting/setting Eigen
 * objects, double, int and bool types.
 *
 */
class ZmqClient {
public:

	ZmqClient() = default;
	ZmqClient(const ZmqClient&) = delete;
	ZmqClient& operator=(const ZmqClient&) = delete;

	ZmqClient(const std::string& key_namespace_prefix);

	/**
	 * @brief Connect to ZMQ REP Server.
	 *
	 * @param hostname  Server IP address (default 127.0.0.1).
	 * @param port      Server port number (default 5555).
	 * @param timeout   Connection receive timeout.
	 */
	void connect(const std::string& hostname = "127.0.0.1",
				 const int port = 5555,
				 const struct timeval& timeout = {1, 500000});

	/**
	 * @brief Perform Command: PING.
	 */
	void ping();

	/**
	 * @brief Perform Command: GET key and returns as a string
	 */
	std::string get(const std::string& key);

	inline double getDouble(const std::string& key) {
		return std::stod(get(key));
	}

	inline int getInt(const std::string& key) { return std::stoi(get(key)); }

	inline bool getBool(const std::string& key) {
		return (bool)std::stoi(get(key));
	}

	inline Eigen::MatrixXd getEigen(const std::string& key) {
		return decodeEigenMatrix(get(key));
	}

	/**
	 * @brief Perform Command: SET key value.
	 */
	void set(const std::string& key, const std::string& value);

	/**
	 * @brief Perform Command: SET key value, with binary data.
	 */
	void set(const std::string& key, const std::vector<unsigned char>& value);

	inline void setDouble(const std::string& key, const double& value) {
		set(key, std::to_string(value));
	}

	inline void setInt(const std::string& key, const int& value) {
		set(key, std::to_string(value));
	}

	inline void setBool(const std::string& key, const bool& value) {
		value ? set(key, "1") : set(key, "0");
	}

	template <typename Derived>
	inline void setEigen(const std::string& key,
						 const Eigen::MatrixBase<Derived>& value) {
		set(key, encodeEigenMatrix(value));
	}

	/**
	 * @brief Perform Command: DEL key to delete a key
	 */
	void del(const std::string& key);

	/**
	 * Perform Command: EXISTS key to check if a key exists
	 */
	bool exists(const std::string& key);

	void createNewSendGroup(const std::string& group_name);
	void createNewReceiveGroup(const std::string& group_name);

	void deleteSendGroup(const std::string& group_name);
	void deleteReceiveGroup(const std::string& group_name);

	void addToReceiveGroup(const std::string& key, double& object,
						   const std::string& group_name = "default");
	void addToReceiveGroup(const std::string& key, std::string& object,
						   const std::string& group_name = "default");
	void addToReceiveGroup(const std::string& key, int& object,
						   const std::string& group_name = "default");
	void addToReceiveGroup(const std::string& key, bool& object,
						   const std::string& group_name = "default");
	template <typename _Scalar, int _Rows, int _Cols, int _Options,
			  int _MaxRows, int _MaxCols>
	void addToReceiveGroup(const std::string& key,
						   Eigen::Matrix<_Scalar, _Rows, _Cols, _Options,
										 _MaxRows, _MaxCols>& object,
						   const std::string& group_name = "default");

	void addToSendGroup(const std::string& key, const double& object,
						const std::string& group_name = "default");
	void addToSendGroup(const std::string& key, const std::string& object,
						const std::string& group_name = "default");
	void addToSendGroup(const std::string& key, const int& object,
						const std::string& group_name = "default");
	void addToSendGroup(const std::string& key, const bool& object,
						const std::string& group_name = "default");
	template <typename _Scalar, int _Rows, int _Cols, int _Options,
			  int _MaxRows, int _MaxCols>
	void addToSendGroup(const std::string& key,
						const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options,
											_MaxRows, _MaxCols>& object,
						const std::string& group_name = "default");

	void receiveAllFromGroup(const std::string& group_name = "default");
	void receiveAllFromGroup(const std::vector<std::string>& group_names);

	void sendAllFromGroup(const std::string& group_name = "default");
	void sendAllFromGroup(const std::vector<std::string>& group_names);

private:
	enum SupportedTypes {
		INT_NUMBER,
		DOUBLE_NUMBER,
		BOOL,
		STRING,
		EIGEN_OBJECT,
	};

	/**
	 * Issue a multi-part command via ZeroMQ
	 */
	std::vector<std::string> request(const std::vector<std::string>& args);

	template <typename Derived>
	static std::string encodeEigenMatrix(
		const Eigen::MatrixBase<Derived>& matrix);

	static Eigen::MatrixXd decodeEigenMatrix(const std::string& str);

	std::vector<std::string> pipeget(const std::vector<std::string>& keys);
	void pipeset(const std::vector<std::pair<std::string, std::string>>& keyvals);

	std::vector<std::string> mget(const std::vector<std::string>& keys);
	void mset(const std::vector<std::pair<std::string, std::string>>& keyvals);

	bool sendGroupExists(const std::string& group_name) const;
	bool receiveGroupExists(const std::string& group_name) const;

	std::unique_ptr<zmq::context_t> _context;
	std::unique_ptr<zmq::socket_t> _socket;

	std::vector<std::string> _receive_group_names;
	std::map<std::string, std::vector<std::string>> _keys_to_receive;
	std::map<std::string, std::vector<void*>> _objects_to_receive;
	std::map<std::string, std::vector<SupportedTypes>>
		_objects_to_receive_types;

	std::vector<std::string> _send_group_names;
	std::map<std::string, std::vector<std::string>> _keys_to_send;
	std::map<std::string, std::vector<const void*>> _objects_to_send;
	std::map<std::string, std::vector<SupportedTypes>>
		_objects_to_send_types;
	std::map<std::string, std::vector<std::pair<int, int>>>
		_objects_to_send_sizes;

	std::string _prefix = "";
	std::string _host = "";
	int _port = 0;
};

template <typename Derived>
std::string ZmqClient::encodeEigenMatrix(
	const Eigen::MatrixBase<Derived>& matrix) {
	std::string s = "[";
	if (matrix.cols() == 1) {  
		for (int i = 0; i < matrix.rows(); ++i) {
			if (i > 0) s.append(",");
			s.append(std::to_string(matrix(i, 0)));
		}
	} else { 
		for (int i = 0; i < matrix.rows(); ++i) {
			if (i > 0) s.append(",");
			if (matrix.rows() > 1) s.append("[");
			for (int j = 0; j < matrix.cols(); ++j) {
				if (j > 0) s.append(",");
				s.append(std::to_string(matrix(i, j)));
			}
			if (matrix.rows() > 1) s.append("]");
		}
	}
	s.append("]");
	return s;
}

template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
		  int _MaxCols>
void ZmqClient::addToReceiveGroup(
	const std::string& key,
	Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& object,
	const std::string& group_name) {
	if (!receiveGroupExists(group_name)) {
		throw std::runtime_error(
			"Receive group with that name not found, cannot add object to "
			"receive");
	}

	setEigen(key, object);
	_keys_to_receive[group_name].push_back(key);
	_objects_to_receive[group_name].push_back(object.data());
	_objects_to_receive_types[group_name].push_back(EIGEN_OBJECT);
}

template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
		  int _MaxCols>
void ZmqClient::addToSendGroup(
	const std::string& key,
	const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>&
		object,
	const std::string& group_name) {
	if (!sendGroupExists(group_name)) {
		throw std::runtime_error(
			"Send group with that name not found, cannot add object to send");
	}

	_keys_to_send[group_name].push_back(key);
	_objects_to_send[group_name].push_back(object.data());
	_objects_to_send_types[group_name].push_back(EIGEN_OBJECT);
	_objects_to_send_sizes[group_name].push_back(
		std::make_pair(object.rows(), object.cols()));
}

}  // namespace SaiCommon

#endif	// ZMQ_CLIENT_H