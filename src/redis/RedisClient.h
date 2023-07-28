/**
 * RedisClient.h
 *
 * Author: Toki Migimatsu
 *         Shameek Ganguly
 * Created: April 2017
 */

#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <hiredis/hiredis.h>

#include <Eigen/Core>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>

namespace Sai2Common {

namespace RedisServer {
// Default server ip
const std::string DEFAULT_IP = "127.0.0.1";

// Default server port
const int DEFAULT_PORT = 6379;

// Default Redis key prefix
static const std::string KEY_PREFIX = "sai2::";
}  // namespace RedisServer

struct redisReplyDeleter {
	void operator()(redisReply* r) { freeReplyObject(r); }
};

struct redisContextDeleter {
	void operator()(redisContext* c) { redisFree(c); }
};

class RedisClient {
public:
	/**
	 * Connect to Redis server.
	 *
	 * @param hostname  Redis server IP address (default 127.0.0.1).
	 * @param port      Redis server port number (default 6379).
	 * @param timeout   Connection attempt timeout (default 1.5s).
	 */
	void connect(const std::string& hostname = "127.0.0.1",
				 const int port = 6379,
				 const struct timeval& timeout = {1, 500000});

	/**
	 * Perform Redis command: PING.
	 *
	 * If the server is responsive, it should reply PONG.
	 */
	void ping();

	/**
	 * Perform Redis command: GET key.
	 *
	 * @param key  Key to get from Redis (entry must be String type).
	 * @return     String value.
	 */
	std::string get(const std::string& key);
	inline double getDouble(const std::string& key) {
		return std::stod(get(key));
	}
	inline int getInt(const std::string& key) { return std::stoi(get(key)); }
	inline Eigen::MatrixXd getEigen(const std::string& key) {
		return decodeEigenMatrix(get(key));
	}

	/**
	 * Perform Redis command: SET key value.
	 *
	 * @param key    Key to set in Redis.
	 * @param value  Value for key.
	 */
	void set(const std::string& key, const std::string& value);
	inline void setDouble(const std::string& key, const double& value) {
		set(key, std::to_string(value));
	}
	inline void setInt(const std::string& key, const int& value) {
		set(key, std::to_string(value));
	}
	template <typename Derived>
	inline void setEigen(const std::string& key,
						 const Eigen::MatrixBase<Derived>& value) {
		set(key, encodeEigenMatrix(value));
	}

	/**
	 * Perform Redis command: DEL key.
	 *
	 * @param key    Key to delete in Redis.
	 */
	void del(const std::string& key);

	/**
	 * Perform Redis command: EXISTS key.
	 *
	 * @param key    Key to delete in Redis.
	 * @return       true if key exists, false otherwise.
	 */
	bool exists(const std::string& key);

	void createNewSendGroup(const int group_number);
	void createNewReceiveGroup(const int group_number);

	void addToReceiveGroup(const std::string& key, double& object,
						   const int group_number = 0);
	void addToReceiveGroup(const std::string& key, std::string& object,
						   const int group_number = 0);
	void addToReceiveGroup(const std::string& key, int& object,
						   const int group_number = 0);
	template <typename _Scalar, int _Rows, int _Cols, int _Options,
			  int _MaxRows, int _MaxCols>
	void addToReceiveGroup(const std::string& key,
						   Eigen::Matrix<_Scalar, _Rows, _Cols, _Options,
										 _MaxRows, _MaxCols>& object,
						   const int group_number = 0);

	void addToSendGroup(const std::string& key, const double& object,
						const int group_number = 0);
	void addToSendGroup(const std::string& key, const std::string& object,
						const int group_number = 0);
	void addToSendGroup(const std::string& key, const int& object,
						const int group_number = 0);
	template <typename _Scalar, int _Rows, int _Cols, int _Options,
			  int _MaxRows, int _MaxCols>
	void addToSendGroup(const std::string& key,
						const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options,
											_MaxRows, _MaxCols>& object,
						const int group_number = 0);

	void receiveAllFromGroup(const int group_number = 0);
	void sendAllFromGroup(const int group_number = 0);

private:
	/**
	 * private variables for automating pipeget and pipeset
	 */
	enum RedisSupportedTypes {
		INT_NUMBER,
		DOUBLE_NUMBER,
		STRING,
		EIGEN_OBJECT,
	};

	/**
	 * Issue a command to Redis.
	 *
	 * This function is a C++ wrapper around hiredis::redisCommand() that
	 * provides a self-freeing redisReply pointer. The command is formatted in
	 * printf() style.
	 *
	 * @param format  Format string.
	 * @param ...     Format values.
	 * @return        redisReply pointer.
	 */
	std::unique_ptr<redisReply, redisReplyDeleter> command(const char* format,
														   ...);

	/**
	 * Encode Eigen::MatrixXd as JSON.
	 *
	 * encodeEigenMatrixJSON():
	 *   [1,2,3,4]     => "[1,2,3,4]"
	 *   [[1,2],[3,4]] => "[[1,2],[3,4]]"
	 *
	 * @param matrix  Eigen::MatrixXd to encode.
	 * @return        Encoded string.
	 */
	template <typename Derived>
	static std::string encodeEigenMatrix(
		const Eigen::MatrixBase<Derived>& matrix);

	/**
	 * Decode Eigen::MatrixXd from JSON.
	 *
	 * decodeEigenMatrixJSON():
	 *   "[1,2,3,4]"     => [1,2,3,4]
	 *   "[[1,2],[3,4]]" => [[1,2],[3,4]]
	 *
	 * @param str  String to decode.
	 * @return     Decoded Eigen::Matrix. Optimized with RVO.
	 */
	static Eigen::MatrixXd decodeEigenMatrix(const std::string& str);

	/**
	 * Perform Redis GET commands in bulk: GET key1; GET key2...
	 *
	 * Pipeget gets multiple keys as a non-atomic operation. More efficient than
	 * getting the keys separately. See:
	 * https://redis.io/topics/mass-insert
	 *
	 * In C++11, this function can be called with brace initialization:
	 * auto values = redis_client.pipeget({"key1", "key2"});
	 *
	 * @param keys  Vector of keys to get from Redis.
	 * @return      Vector of retrieved values. Optimized with RVO.
	 */
	std::vector<std::string> pipeget(const std::vector<std::string>& keys);

	/**
	 * Perform Redis SET commands in bulk: SET key1 val1; SET key2 val2...
	 *
	 * Pipeset sets multiple keys as a non-atomic operation. More efficient than
	 * setting the keys separately. See:
	 * https://redis.io/topics/mass-insert
	 *
	 * In C++11, this function can be called with brace initialization:
	 * redis_client.pipeset({{"key1", "val1"}, {"key2", "val2"}});
	 *
	 * @param keyvals  Vector of key-value pairs to set in Redis.
	 */
	void pipeset(
		const std::vector<std::pair<std::string, std::string>>& keyvals);

	/**
	 * Perform Redis command: MGET key1 key2...
	 *
	 * MGET gets multiple keys as an atomic operation. See:
	 * https://redis.io/commands/mget
	 *
	 * @param keys  Vector of keys to get from Redis.
	 * @return      Vector of retrieved values. Optimized with RVO.
	 */
	std::vector<std::string> mget(const std::vector<std::string>& keys);

	/**
	 * Perform Redis command: MSET key1 val1 key2 val2...
	 *
	 * MSET sets multiple keys as an atomic operation. See:
	 * https://redis.io/commands/mset
	 *
	 * @param keyvals  Vector of key-value pairs to set in Redis.
	 */
	void mset(const std::vector<std::pair<std::string, std::string>>& keyvals);

	std::unique_ptr<redisContext, redisContextDeleter> context_;

	std::vector<int> _receive_group_indexes;
	std::vector<std::vector<std::string>> _keys_to_receive;
	std::vector<std::vector<void*>> _objects_to_receive;
	std::vector<std::vector<RedisSupportedTypes>> _objects_to_receive_types;

	std::vector<int> _send_group_indexes;
	std::vector<std::vector<std::string>> _keys_to_send;
	std::vector<std::vector<const void*>> _objects_to_send;
	std::vector<std::vector<RedisSupportedTypes>> _objects_to_send_types;
	std::vector<std::vector<std::pair<int, int>>> _objects_to_send_sizes;
};

// Implementation must be part of header for compile time template
// specialization
template <typename Derived>
std::string RedisClient::encodeEigenMatrix(
	const Eigen::MatrixBase<Derived>& matrix) {
	std::string s = "[";
	if (matrix.cols() == 1) {  // Column vector
		// [[1],[2],[3],[4]] => "[1,2,3,4]"
		for (int i = 0; i < matrix.rows(); ++i) {
			if (i > 0) s.append(",");
			s.append(std::to_string(matrix(i, 0)));
		}
	} else {  // Matrix
		// [[1,2,3,4]]   => "[1,2,3,4]"
		// [[1,2],[3,4]] => "[[1,2],[3,4]]"
		for (int i = 0; i < matrix.rows(); ++i) {
			if (i > 0) s.append(",");
			// Nest arrays only if there are multiple rows
			if (matrix.rows() > 1) s.append("[");
			for (int j = 0; j < matrix.cols(); ++j) {
				if (j > 0) s.append(",");
				s.append(std::to_string(matrix(i, j)));
			}
			// Nest arrays only if there are multiple rows
			if (matrix.rows() > 1) s.append("]");
		}
	}
	s.append("]");
	return s;
}

template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
		  int _MaxCols>
void RedisClient::addToReceiveGroup(
	const std::string& key,
	Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& object,
	const int group_number) {
	int n = _receive_group_indexes.size();
	int group_index = 0;
	bool found = false;
	while (group_index < n) {
		if (_receive_group_indexes[group_index] == group_number) {
			found = true;
			break;
		}
		group_index++;
	}
	if (!found) {
		throw std::runtime_error(
			"no read callback with this index in "
			"RedisClient::addToReceiveGroup("
			"const std::string& key, Eigen::Matrix< _Scalar, _Rows, _Cols, "
			"_Options, _MaxRows, _MaxCols >& object)\n");
	}

	_keys_to_receive[group_index].push_back(key);
	_objects_to_receive[group_index].push_back(object.data());
	_objects_to_receive_types[group_index].push_back(EIGEN_OBJECT);
}

template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
		  int _MaxCols>
void RedisClient::addToSendGroup(
	const std::string& key,
	const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>&
		object,
	const int group_number) {
	int n = _send_group_indexes.size();
	int group_index = 0;
	bool found = false;
	while (group_index < n) {
		if (_send_group_indexes[group_index] == group_number) {
			found = true;
			break;
		}
		group_index++;
	}
	if (!found) {
		throw std::runtime_error(
			"no write group with this index in "
			"RedisClient::addToSendGroup("
			"const std::string& key, Eigen::Matrix< _Scalar, _Rows, _Cols, "
			"_Options, _MaxRows, _MaxCols >& object)\n");
	}

	_keys_to_send[group_index].push_back(key);
	_objects_to_send[group_index].push_back(object.data());
	_objects_to_send_types[group_index].push_back(EIGEN_OBJECT);
	_objects_to_send_sizes[group_index].push_back(
		std::make_pair(object.rows(), object.cols()));
}

}  // namespace Sai2Common

#endif	// REDIS_CLIENT_H