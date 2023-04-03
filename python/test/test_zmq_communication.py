import concurrent.futures
import numpy as np
import state_representation as sr
import time
import unittest
import zmq
from network_interfaces.zmq import network
from numpy.testing import assert_array_almost_equal


class TestZMQNetworkInterface(unittest.TestCase):
    robot_state = None
    robot_joint_state = None
    robot_jacobian = None
    robot_mass = None
    control_command = None
    control_type = None
    context = None
    received_state = None
    received_command = None

    @classmethod
    def setUpClass(cls):
        cls.robot_state = sr.CartesianState().Random("ee", "robot")
        cls.robot_joint_state = sr.JointState().Random("robot", 3)
        cls.robot_jacobian = sr.Jacobian().Random("robot", 3, "frame")
        cls.robot_mass = sr.Parameter("mass", np.random.rand(3, 3), sr.ParameterType.MATRIX)
        cls.robot_external_wrench = sr.CartesianWrench().Random("ee", "robot")
        cls.robot_external_torque = sr.Parameter("external_torque", np.random.rand(7, 1), sr.ParameterType.VECTOR)
        cls.control_command = sr.JointState().Random("robot", 3)
        cls.control_type = [1, 2, 3]
        cls.context = zmq.Context()

    def assert_state_equal(self, state1, state2):
        self.assertTrue(state1.is_compatible(state2))
        assert_array_almost_equal(state1.data(), state2.data())

    def robot(self):
        command_subscriber, state_publisher = network.configure_sockets(self.context, "127.0.0.1:1702",
                                                                        "127.0.0.1:1701")

        state = network.StateMessage(self.robot_state, self.robot_joint_state, self.robot_jacobian, self.robot_mass, self.robot_external_wrench, self.robot_external_torque)
        command = []
        start_time = time.time()
        while time.time() - start_time < 2.:
            network.send_state(state, state_publisher)
            command = network.receive_command(command_subscriber)
            time.sleep(0.01)
        command_subscriber.close()
        state_publisher.close()
        self.received_command = command

    def control(self):
        state_subscriber, command_publisher = network.configure_sockets(self.context, "127.0.0.1:1701",
                                                                        "127.0.0.1:1702")

        command = network.CommandMessage(self.control_type, self.control_command)
        state = []
        start_time = time.time()
        while time.time() - start_time < 2.:
            network.send_command(command, command_publisher)
            state = network.receive_state(state_subscriber)
            time.sleep(0.01)
        state_subscriber.close()
        command_publisher.close()
        self.received_state = state

    def test_communication(self):
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as e:
            e.submit(self.robot)
            e.submit(self.control)

        [self.assertEqual(self.received_command.control_type[i], self.control_type[i]) for i in
         range(len(self.control_type))]
        self.assert_state_equal(self.received_command.joint_state, self.control_command)

        self.assert_state_equal(self.received_state.ee_state, self.robot_state)
        self.assert_state_equal(self.received_state.joint_state, self.robot_joint_state)
        self.assert_state_equal(self.received_state.jacobian, self.robot_jacobian)
        self.assertTrue(self.received_state.mass.is_compatible(self.robot_mass))
        assert_array_almost_equal(self.received_state.mass.get_value(), self.robot_mass.get_value())
        self.assert_state_equal(self.received_state.external_wrench, self.robot_external_wrench)
        self.assertTrue(self.received_state.external_torque.is_compatible(self.robot_external_torque))
        assert_array_almost_equal(self.received_state.external_torque.get_value(), self.robot_external_torque.get_value())

    def test_encode_command(self):
        command = network.CommandMessage([], sr.JointState())
        network.encode_command(command)

        command = network.CommandMessage([], sr.JointState().Random("test", 3))
        with self.assertRaises(ValueError):
            network.encode_command(command)

        command.control_type = [1]
        encoded = network.encode_command(command)
        decoded = network.decode_command(encoded)
        self.assertEqual(len(decoded.control_type), 3)
        assert_array_almost_equal(decoded.control_type, [1, 1, 1])

        command.control_type = [1, 2, 6]
        with self.assertRaises(ValueError):
            network.encode_command(command)

        command.control_type = [1, 2, 2, 1]
        with self.assertRaises(ValueError):
            network.encode_command(command)
