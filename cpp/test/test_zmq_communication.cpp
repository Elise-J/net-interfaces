#include <gtest/gtest.h>
#include <thread>

#include "network_interfaces/zmq/network.h"
#include "network_interfaces/zmq/exceptions/InvalidControlTypeVectorException.h"
#include "network_interfaces/zmq/exceptions/UnknownControlTypeException.h"

static void
expect_joint_state_equal(const state_representation::JointState& js1, const state_representation::JointState& js2) {
  EXPECT_TRUE(js1.data().isApprox(js2.data()));
  EXPECT_TRUE(js1.is_compatible(js2));
}

static void expect_cart_state_equal(
    const state_representation::CartesianState& cs1, const state_representation::CartesianState& cs2
) {
  EXPECT_TRUE(cs1.data().isApprox(cs2.data()));
  EXPECT_TRUE(cs1.is_compatible(cs2));
}

class TestNetworkInterface : public ::testing::Test {
public:
  TestNetworkInterface() {
    robot_state = state_representation::CartesianState::Random("ee", "robot");
    robot_joint_state = state_representation::JointState::Random("robot", 3);
    robot_jacobian = state_representation::Jacobian::Random("robot", 3, "frame");
    robot_mass.set_value(Eigen::MatrixXd::Random(3, 3));
    robot_external_wrench = state_representation::CartesianWrench::Random("ee", "robot");
    robot_external_torque.set_value(Eigen::MatrixXd::Random(7, 1));
    control_joint_state = state_representation::JointState::Random("robot", 3);
    control_type = std::vector<int>{1, 2, 3};
  }

  state_representation::CartesianState robot_state;
  state_representation::JointState robot_joint_state;
  state_representation::Jacobian robot_jacobian;
  state_representation::Parameter<Eigen::MatrixXd>
  robot_mass = state_representation::Parameter<Eigen::MatrixXd>("robot");
  state_representation::CartesianWrench robot_external_wrench;
  state_representation::Parameter<Eigen::MatrixXd>
  robot_external_torque = state_representation::Parameter<Eigen::MatrixXd>("robot");
  state_representation::JointState control_joint_state;
  std::vector<int> control_type;

  std::thread spawn_robot() {
    return std::thread([this] { this->robot(); });
  }

  void robot() const {
    zmq::context_t context = zmq::context_t(1);
    zmq::socket_t state_publisher, command_subscriber;
    network_interfaces::zmq::configure_sockets(
        context, command_subscriber, "127.0.0.1:1702", state_publisher, "127.0.0.1:1701"
    );

    network_interfaces::zmq::CommandMessage command;
    network_interfaces::zmq::StateMessage state;
    state.ee_state = robot_state;
    state.joint_state = robot_joint_state;
    state.jacobian = robot_jacobian;
    state.mass = robot_mass;
    state.external_wrench = robot_external_wrench;
    state.external_torque = robot_external_torque;
    for (std::size_t i = 0; i < 100; ++i) {
      network_interfaces::zmq::send(state, state_publisher);
      network_interfaces::zmq::receive(command, command_subscriber);
      usleep(10000);
    }
    expect_joint_state_equal(command.joint_state, control_joint_state);
    for (std::size_t i = 0; i < control_type.size(); ++i) {
      EXPECT_EQ(control_type.at(i), command.control_type.at(i));
    }
  }

  std::thread spawn_control() {
    return std::thread([this] { this->control(); });
  }

  void control() const {
    zmq::context_t context = zmq::context_t(1);
    zmq::socket_t command_publisher, state_subscriber;
    network_interfaces::zmq::configure_sockets(
        context, state_subscriber, "127.0.0.1:1701", command_publisher, "127.0.0.1:1702"
    );

    network_interfaces::zmq::StateMessage state;
    network_interfaces::zmq::CommandMessage command;
    command.joint_state = control_joint_state;
    command.control_type = control_type;
    for (std::size_t i = 0; i < 100; ++i) {
      network_interfaces::zmq::send(command, command_publisher);
      network_interfaces::zmq::receive(state, state_subscriber);
      usleep(10000);
    }
    expect_cart_state_equal(state.ee_state, robot_state);
    expect_joint_state_equal(state.joint_state, robot_joint_state);
    EXPECT_TRUE(state.jacobian.data().isApprox(robot_jacobian.data()));
    EXPECT_TRUE(state.jacobian.is_compatible(robot_jacobian));
    EXPECT_TRUE(state.mass.get_value().isApprox(robot_mass.get_value()));
    EXPECT_TRUE(state.mass.is_compatible(robot_mass));


    EXPECT_TRUE(state.external_wrench.data().isApprox(robot_external_wrench.data()));
    EXPECT_TRUE(state.external_wrench.is_compatible(robot_external_wrench));

    EXPECT_TRUE(state.external_torque.get_value().isApprox(robot_external_torque.get_value()));
    EXPECT_TRUE(state.external_torque.is_compatible(robot_external_torque));
  }
};

TEST_F(TestNetworkInterface, TestEncodeCommand) {
  network_interfaces::zmq::CommandMessage command;
  command.joint_state = state_representation::JointState::Random("test", 3);
  command.control_type = std::vector<int>{};
  EXPECT_THROW(network_interfaces::zmq::encode_command(command),
               network_interfaces::zmq::exceptions::InvalidControlTypeVectorException);

  command.control_type = std::vector<int>{1};
  network_interfaces::zmq::encode_command(command);
  ASSERT_EQ(command.control_type.size(), 3);
  for (std::size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(command.control_type.at(i), 1);
  }

  command.control_type = std::vector<int>{1, 2, 6};
  EXPECT_THROW(network_interfaces::zmq::encode_command(command),
               network_interfaces::zmq::exceptions::UnknownControlTypeException);

  command.control_type = std::vector<int>{1, 2, 2, 1};
  EXPECT_THROW(network_interfaces::zmq::encode_command(command),
               network_interfaces::zmq::exceptions::InvalidControlTypeVectorException);
}

TEST_F(TestNetworkInterface, TestEmptyEncodeCommand) {
  network_interfaces::zmq::CommandMessage command;
  EXPECT_NO_THROW(network_interfaces::zmq::encode_command(command));

  command.control_type = std::vector<int>{1};
  EXPECT_THROW(network_interfaces::zmq::encode_command(command),
               network_interfaces::zmq::exceptions::InvalidControlTypeVectorException);
}

TEST_F(TestNetworkInterface, TestCommunication) {
  std::thread robot = spawn_robot();
  std::thread control = spawn_control();

  robot.join();
  control.join();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
