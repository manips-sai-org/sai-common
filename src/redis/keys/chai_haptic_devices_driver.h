#include <string>

using namespace std;

namespace SaiCommon {
namespace ChaiHapticDriverKeys {
const string CHAI_REDIS_DRIVER_NAMESPACE = "chai_haptic_devices_driver";

const string MAX_STIFFNESS_KEY_SUFFIX = "specifications::max_stiffness";
const string MAX_DAMPING_KEY_SUFFIX = "specifications::max_damping";
const string MAX_FORCE_KEY_SUFFIX = "specifications::max_force";
const string MAX_GRIPPER_ANGLE = "specifications::max_gripper_angle";
const string COMMANDED_FORCE_KEY_SUFFIX = "actuators::commanded_force";
const string COMMANDED_TORQUE_KEY_SUFFIX = "actuators::commanded_torque";
const string COMMANDED_GRIPPER_FORCE_KEY_SUFFIX =
	"actuators::commanded_force_gripper";
const string POSITION_KEY_SUFFIX = "sensors::current_position";
const string ROTATION_KEY_SUFFIX = "sensors::current_rotation";
const string GRIPPER_POSITION_KEY_SUFFIX =
	"sensors::current_position_gripper";
const string LINEAR_VELOCITY_KEY_SUFFIX = "sensors::current_trans_velocity";
const string ANGULAR_VELOCITY_KEY_SUFFIX = "sensors::current_rot_velocity";
const string GRIPPER_VELOCITY_KEY_SUFFIX =
	"sensors::current_velocity_gripper";
const string SENSED_FORCE_KEY_SUFFIX = "sensors::sensed_force";
const string SENSED_TORQUE_KEY_SUFFIX = "sensors::sensed_torque";
const string USE_GRIPPER_AS_SWITCH_KEY_SUFFIX =
	"parametrization::use_gripper_as_switch";
const string SWITCH_PRESSED_KEY_SUFFIX = "sensors::switch_pressed";
const string DRIVER_RUNNING_KEY_SUFFIX = "driver_running";

const string HAPTIC_DEVICES_SWAP_KEY = "ChaiHapticDevice::swapDevices";


string createRedisKey(const string& key_suffix, int device_number) {
	return CHAI_REDIS_DRIVER_NAMESPACE + "::device" + to_string(device_number) + "::" + key_suffix;
}

}  // namespace ChaiHapticDriverKeys
}  // namespace SaiCommon