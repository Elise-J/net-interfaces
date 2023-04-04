from dataclasses import dataclass
from typing import List

import clproto
import state_representation as sr
import zmq

from network_interfaces.control_type import ControlType


@dataclass
class StateMessage:
    """
    A collection of state variables in Cartesian and joint space that the robot publishes.
    """
    ee_state: sr.CartesianState
    joint_state: sr.JointState
    jacobian: sr.Jacobian
    mass: sr.Parameter("mass", sr.ParameterType.MATRIX)
    external_wrench : sr.CartesianWrench
    external_torque: sr.Parameter("external_torque", sr.ParameterType.VECTOR)

    def __init__(self, ee_state=sr.CartesianState(), joint_state=sr.JointState(), jacobian=sr.Jacobian(),
                 mass=sr.Parameter("mass", sr.ParameterType.MATRIX), external_wrench=sr.CartesianWrench(),
                 external_torque=sr.Parameter("external_torque", sr.ParameterType.VECTOR)):
        assert isinstance(ee_state, sr.CartesianState)
        assert isinstance(joint_state, sr.JointState)
        assert isinstance(jacobian, sr.Jacobian)
        assert isinstance(mass, sr.Parameter)
        assert isinstance(external_wrench, sr.CartesianWrench)
        assert isinstance(external_torque, sr.Parameter)
        self.ee_state = ee_state
        self.joint_state = joint_state
        self.jacobian = jacobian
        self.mass = mass
        self.external_wrench = external_wrench
        self.external_torque = external_torque


@dataclass
class CommandMessage:
    """
    A collection of state variables in joint space that the robot follows according
    to the specified control type.
    """
    control_type: List[int]
    joint_state: sr.JointState

    def __init__(self, control_type=None, joint_state=sr.JointState()):
        if control_type is None:
            control_type = []
        assert isinstance(control_type, list)
        assert isinstance(joint_state, sr.JointState)
        self.control_type = control_type
        self.joint_state = joint_state


def encode_state(state):
    """
    Encode a state message into the serialized binary format.

    :param state: The StateMessage to encode
    :type state: StateMessage
    :return: An ordered vector of encoded strings representing the state message fields
    :rtype: list of str
    """
    encoded_state = list()
    encoded_state.append(clproto.encode(state.ee_state, clproto.MessageType.CARTESIAN_STATE_MESSAGE))
    encoded_state.append(clproto.encode(state.joint_state, clproto.MessageType.JOINT_STATE_MESSAGE))
    encoded_state.append(clproto.encode(state.jacobian, clproto.MessageType.JACOBIAN_MESSAGE))
    encoded_state.append(clproto.encode(state.mass, clproto.MessageType.PARAMETER_MESSAGE))
    encoded_state.append(clproto.encode(state.external_wrench, clproto.MessageType.CARTESIAN_WRENCH_MESSAGE))
    encoded_state.append(clproto.encode(state.external_torque, clproto.MessageType.PARAMETER_MESSAGE))
    return encoded_state


def encode_command(command):
    """
    Encode a command message into the serialized binary format.

    :param command: The CommandMessage to encode
    :type command: CommandMessage
    :return: An ordered vector of encoded strings representing the command message fields
    :rtype: list of str
    """
    if len(command.control_type) != command.joint_state.get_size():
        if len(command.control_type) == 1 and not command.joint_state.is_empty():
            command.control_type = [command.control_type[0]] * command.joint_state.get_size()
        else:
            raise ValueError("The size of field 'control_type' of the CommandMessage does not correspond"
                             " to the size of the field 'joint state'.")
    for i in range(command.joint_state.get_size()):
        if command.control_type[i] >= ControlType.END.value:
            raise ValueError("The desired 'control_type' of the CommandMessage is unknown.")
    encoded_command = list()
    control_type_param = sr.Parameter("control_type", command.control_type, sr.ParameterType.INT_ARRAY)
    encoded_command.append(clproto.encode(control_type_param, clproto.MessageType.PARAMETER_MESSAGE))
    encoded_command.append(clproto.encode(command.joint_state, clproto.MessageType.JOINT_STATE_MESSAGE))
    return encoded_command


def decode_state(message):
    """
    Decode a state message from the serialized binary format.

    :param message: An ordered vector of encoded strings representing the state message fields
    :type message: list of str
    :return: The equivalent StateMessage
    :rtype: StateMessage
    """
    state = StateMessage(clproto.decode(message[0]), clproto.decode(message[1]), clproto.decode(message[2]),
                         clproto.decode(message[3]), clproto.decode(message[4]), clproto.decode(message[5]))
    return state


def decode_command(message):
    """
    Decode a command message from the serialized binary format.

    :param message: An ordered vector of encoded strings representing the command message fields
    :type message: list of str
    :return: The equivalent CommandMessage
    :rtype: CommandMessage
    """
    command = CommandMessage(clproto.decode(message[0]).get_value(), clproto.decode(message[1]))
    return command


def configure_subscriber(context, subscriber_uri, bind=False):
    """
    Configure the zmq subscriber socket to receive messages.

    :param context: The ZMQ context object
    :param subscriber_uri: The URI (IP:Port) of the network socket
    :param bind: Optional flag to decide if subscriber should bind (True) or connect (False)
    :type context: zmq.Context
    :type subscriber_uri: str
    :type bind: bool
    :return: The subscription socket that is used to receive messages
    :rtype: zmq.Socket
    """
    subscriber = context.socket(zmq.SUB)
    subscriber.setsockopt(zmq.CONFLATE, 1)
    subscriber.setsockopt_string(zmq.SUBSCRIBE, "")
    if bind:
        subscriber.bind("tcp://" + subscriber_uri)
    else:
        subscriber.connect("tcp://" + subscriber_uri)
    return subscriber


def configure_publisher(context, publisher_uri, bind=True):
    """
    Configure the zmq publisher socket to publish messages.

    :param context: The ZMQ context object
    :param publisher_uri: The URI (IP:Port) of the network socket
    :param bind: Optional flag to decide if publisher should bind (True) or connect (False)
    :type context: zmq.Context
    :type publisher_uri: str
    :return: The publication socket that is used to send messages
    :rtype: zmq.Socket
    """
    publisher = context.socket(zmq.PUB)
    if bind:
        publisher.bind("tcp://" + publisher_uri)
    else:
        publisher.connect("tcp://" + publisher_uri)
    return publisher


def configure_sockets(context, subscriber_uri, publisher_uri, bind_subscriber=False, bind_publisher=True):
    """
    Configure the zmq sockets to publish and receive messages.

    :param context: The ZMQ context object
    :param subscriber_uri: The URI (IP:Port) of the network socket
    :param publisher_uri: The URI (IP:Port) of the network socket
    :param bind_subscriber: Optional flag to decide if subscriber should bind (True) or connect (False)
    :param bind_publisher: Optional flag to decide if publisher should bind (True) or connect (False)
    :type context: zmq.Context
    :type subscriber_uri: str
    :type publisher_uri: str
    :type bind_subscriber: bool
    :type bind_publisher: bool
    :return: The subscription and publication socket that is used to receive and send messages
    :rtype: tuple[zmq.Socket]
    """
    return configure_subscriber(context, subscriber_uri, bind_subscriber), configure_publisher(context, publisher_uri,
                                                                                               bind_publisher)


def send_encoded_fields(encoded_fields, publisher):
    """
    Send a sequence of encoded field messages.

    :param encoded_fields: An ordered vector of encoded fields
    :param publisher: The configured ZMQ publisher socket
    :type encoded_fields: list of str
    :type publisher: zmq.Socket
    """
    packet = clproto.pack_fields(encoded_fields)
    publisher.send(packet)


def send_state(state, publisher):
    """
    Send a state message.

    :param state: The StateMessage to publish
    :param publisher: The configured ZMQ publisher socket
    :type state: StateMessage
    :type publisher: zmq.Socket
    """
    send_encoded_fields(encode_state(state), publisher)


def send_command(command, publisher):
    """
    Send a command message.

    :param command: The CommandMessage to publish
    :param publisher: The configured ZMQ publisher socket
    :type command: CommandMessage
    :type publisher: zmq.Socket
    """
    send_encoded_fields(encode_command(command), publisher)


def receive_encoded_fields(subscriber, wait=False):
    """
    Receive a sequence of encoded field messages from the socket.

    :param subscriber: The configured ZMQ subscriber socket
    :param wait: Optional flag to decide if the subscriber should wait for a message (True) or continue (False)
    :type subscriber: zmq.Socket
    :type wait: bool
    :return: A vector of encoded fields if a message is available, empty otherwise
    :rtype: list of str
    """
    zmq_flag = 0 if wait else zmq.DONTWAIT
    try:
        message = subscriber.recv(zmq_flag)
    except zmq.error.Again:
        return []

    fields = []
    if message:
        fields = clproto.unpack_fields(message)
    return fields


def receive_state(subscriber, wait=False):
    """
    Receive a state message from the socket.

    :param subscriber: The configured ZMQ subscriber socket
    :param wait: Optional flag to decide if the subscriber should wait for a message (True) or continue (False)
    :type subscriber: zmq.Socket
    :type wait: bool
    :return: The StateMessage object if a message is available
    :rtype: StateMessage
    """
    fields = receive_encoded_fields(subscriber, wait)
    if fields:
        return decode_state(fields)


def receive_command(subscriber, wait=False):
    """
    Receive a command message from the socket.

    :param subscriber: The configured ZMQ subscriber socket
    :param wait: Optional flag to decide if the subscriber should wait for a message (True) or continue (False)
    :type subscriber: zmq.Socket
    :type wait: bool
    :return: The CommandMessage object if a message is available
    :rtype: CommandMessage
    """
    fields = receive_encoded_fields(subscriber, wait)
    if fields:
        return decode_command(fields)
