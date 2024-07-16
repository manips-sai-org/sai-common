/**
 * RedisClient.h
 *
 * Author: 	Toki Migimatsu
 *       	Shameek Ganguly
 *			Mikael Jorda
 * Created: April 2017
 */

#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <hiredis/hiredis.h>

#include <Eigen/Core>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Sai2Common {

namespace RedisServer {
const std::string DEFAULT_IP = "127.0.0.1";
const int DEFAULT_PORT = 6379;
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
	 * @brief Connect to Redis server.
	 *
	 * @param hostname  Redis server IP address (default 127.0.0.1).
	 * @param port      Redis server port number (default 6379).
	 * @param timeout   Connection attempt timeout (default 1.5s).
	 */

	void connect(const std::string& hostname = "127.0.0.1",
				 const int port = 6379,
				 const struct timeval& timeout = {1, 500000});

	/**
	 * @brief Perform Redis command: PING.
	 *
	 * If the server is responsive, it should reply PONG.
	 */
	void ping();

	/**
	 * @brief Perform Redis command: GET key and returns as a string
	 *
	 * @param key  Key to get from Redis (entry must be String type).
	 * @return     value as a string.
	 */
	std::string get(const std::string& key);

	/**
	 * @brief Perform Redis command: GET key and returns as a double
	 *
	 * @param key
	 * @return value converted to a double
	 */
	inline double getDouble(const std::string& key) {
		return std::stod(get(key));
	}

	/**
	 * @brief Perform Redis command: GET key and returns as an int
	 *
	 * @param key
	 * @return value converted to an int
	 */
	inline int getInt(const std::string& key) { return std::stoi(get(key)); }

	/**
	 * @brief Perform Redis command: GET key and returns as a bool (converting
	 * through an int first)
	 *
	 * @param key
	 * @return value converted to a bool
	 */
	inline bool getBool(const std::string& key) {
		return (bool)std::stoi(get(key));
	}

	/**
	 * @brief Perform Redis command: GET key and returns as a matrix or vector
	 * from Eigen library
	 *
	 * @param key
	 * @return value converted to an Eigen object
	 */
	inline Eigen::MatrixXd getEigen(const std::string& key) {
		return decodeEigenMatrix(get(key));
	}

	/**
	 * @brief Perform Redis command: SET key value.
	 *
	 * @param key    Key to set in Redis.
	 * @param value  string value for key.
	 */
	void set(const std::string& key, const std::string& value);

	/**
	 * @brief Perform Redis command: SET key value, with the value converted
	 * from a double
	 *
	 * @param key    Key to set in Redis.
	 * @param value  double value for key.
	 */
	inline void setDouble(const std::string& key, const double& value) {
		set(key, std::to_string(value));
	}

	/**
	 * @brief Perform Redis command: SET key value, with the value converted
	 * from an int
	 *
	 * @param key    Key to set in Redis.
	 * @param value  int value for key.
	 */
	inline void setInt(const std::string& key, const int& value) {
		set(key, std::to_string(value));
	}

	/**
	 * @brief Perform Redis command: SET key value, with the value converted
	 * from a bool to "0" or "1"
	 *
	 * @param key    Key to set in Redis.
	 * @param value  bool value for key.
	 */
	inline void setBool(const std::string& key, const bool& value) {
		value ? set(key, "1") : set(key, "0");
	}

	/**
	 * @brief Perform Redis command: SET key value, with the value converted
	 * from an Eigen MatrixXd or VectorXd object
	 *
	 * @param key    Key to set in Redis.
	 * @param value  Eigen object value for key.
	 */
	template <typename Derived>
	inline void setEigen(const std::string& key,
						 const Eigen::MatrixBase<Derived>& value) {
		set(key, encodeEigenMatrix(value));
	}

	/**
	 * @brief Perform Redis command: DEL key to delete a key
	 *
	 * @param key    Key to delete in Redis.
	 */
	void del(const std::string& key);

	/**
	 * Perform Redis command: EXISTS key to check is a key exists
	 *
	 * @param key    Key to delete in Redis.
	 * @return       true if key exists, false otherwise.
	 */
	bool exists(const std::string& key);

	/**
	 * @brief Create a New Send Group indexed by an int (group 0 is created by
	 * default)
	 *
	 * A send group is a group of keys and object reference to send to the redis
	 * database as a batch (single redis call) each time the
	 * sendAllFromGroup(group_number) function is called
	 *
	 * @param group_number index of the send group to create
	 */
	void createNewSendGroup(const std::string& group_name);

	/**
	 * @brief Create a New Receive Group indexed by an int (group 0 is created
	 * by default)
	 *
	 * Areceive group is a group of keys and references to objects to be
	 * populated by the value from the redis database each time the
	 * receiveAllFromGroup function is called
	 *
	 * @param group_number index of the receive group to create
	 */
	void createNewReceiveGroup(const std::string& group_name);

	/**
	 * @brief delete a send group by name
	 *
	 * @param group_name
	 */
	void deleteSendGroup(const std::string& group_name);

	/**
	 * @brief delete a receive group by name
	 *
	 * @param group_name
	 */
	void deleteReceiveGroup(const std::string& group_name);

	/**
	 * @brief Adds an object to be received in the given group. We can set up
	 * strings, doulbes, ints and Eigen objects to be received that way.
	 *
	 * @param key The redis key of the object
	 * @param object The object reference to populate with the value in the
	 * database corresponding to the key each time the receiveAllFromGroup
	 * function is called with that group number
	 * @param group_number Group number to which to add the object to reveive (0
	 * by default)
	 */
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

	/**
	 * @brief Adds an object to be sent in the given group. We can set up
	 * strings, doulbes, ints and Eigen objects to be sent that way.
	 *
	 * @param key The redis key of the object
	 * @param object The object reference to send to the database for the given
	 * key each time the sendAllFromGroup function is called with that group
	 * number
	 * @param group_number Group number to which to add the object to send (0 by
	 * default)
	 */
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

	/**
	 * @brief update all objects of that group set up via the addToReceiveGroup
	 * with the values from the redis database
	 *
	 * @param group_number
	 */
	void receiveAllFromGroup(const std::string& group_name = "default");
	void receiveAllFromGroup(const std::vector<std::string>& group_names);

	/**
	 * @brief update the redis database with all the objects of that group set
	 * up by the addToSendGroup
	 *
	 * @param group_number
	 */
	void sendAllFromGroup(const std::string& group_name = "default");
	void sendAllFromGroup(const std::vector<std::string>& group_names);

private:
	/**
	 * private variables for automating pipeget and pipeset
	 */
	enum RedisSupportedTypes {
		INT_NUMBER,
		DOUBLE_NUMBER,
		BOOL,
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

	bool sendGroupExists(const std::string& group_name) const;
	bool receiveGroupExists(const std::string& group_name) const;

	/**
	 * @brief redis context pointer
	 *
	 */
	std::unique_ptr<redisContext, redisContextDeleter> _context;

	std::vector<std::string> _receive_group_names;
	std::map<std::string, std::vector<std::string>> _keys_to_receive;
	std::map<std::string, std::vector<void*>> _objects_to_receive;
	std::map<std::string, std::vector<RedisSupportedTypes>>
		_objects_to_receive_types;

	std::vector<std::string> _send_group_names;
	std::map<std::string, std::vector<std::string>> _keys_to_send;
	std::map<std::string, std::vector<const void*>> _objects_to_send;
	std::map<std::string, std::vector<RedisSupportedTypes>>
		_objects_to_send_types;
	std::map<std::string, std::vector<std::pair<int, int>>>
		_objects_to_send_sizes;
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
void RedisClient::addToSendGroup(
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

}  // namespace Sai2Common

#endif	// REDIS_CLIENT_H