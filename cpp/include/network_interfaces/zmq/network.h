#pragma once

#include <clproto.hpp>
#include <state_representation/space/cartesian/CartesianState.hpp>
#include <state_representation/space/joint/JointState.hpp>
#include <state_representation/space/Jacobian.hpp>
#include <state_representation/parameters/Parameter.hpp>
#include <zmq.hpp>

#include "network_interfaces/control_type.h"
#include "network_interfaces/zmq/exceptions/InvalidControlTypeVectorException.h"
#include "network_interfaces/zmq/exceptions/UnknownControlTypeException.h"

namespace network_interfaces::zmq {

/**
 * @struct StateMessage
 * @brief A collection of state variables in Cartesian and joint space that the robot publishes.
 */
struct StateMessage {
  state_representation::CartesianState ee_state;
  state_representation::JointState joint_state;
  state_representation::Jacobian jacobian;
  state_representation::Parameter<Eigen::MatrixXd> mass = state_representation::Parameter<Eigen::MatrixXd>("mass");
  state_representation::CartesianWrench external_wrench;
  state_representation::Parameter<Eigen::MatrixXd> external_torque = state_representation::Parameter<Eigen::MatrixXd>("external_torque");

  inline friend std::ostream& operator<<(std::ostream& os, const StateMessage& message) {
    os << "StateMessage" << std::endl;
    os << message.ee_state << std::endl;
    os << "-" << std::endl;
    os << message.joint_state << std::endl;
    os << "-" << std::endl;
    os << message.jacobian << std::endl;
    os << "-" << std::endl;
    os << message.mass;
    os << "-" << std::endl;
    os << message.external_wrench;
    os << "-" << std::endl;
    os << message.external_torque;
    return os;
  };
};

/**
 * @struct CommandMessage
 * @brief A collection of state variables in joint space that the robot
 * follows according to the specified control type.
 * @see ControlType
 */
struct CommandMessage {
  std::vector<int> control_type;
  state_representation::JointState joint_state;

  inline friend std::ostream& operator<<(std::ostream& os, const CommandMessage& message) {
    os << "CommandMessage" << std::endl;
    for (int i: message.control_type) {
      os << control_type_t(i) << " | ";
    }
    os << std::endl << "-" << std::endl;
    os << message.joint_state;
    return os;
  }
};

// --- Encoding methods --- //

/**
 * @brief Encode a state message into the serialized binary format.
 * @param state The StateMessage to encode
 * @return An ordered vector of encoded strings representing the state message fields
 */
inline std::vector<std::string> encode_state(const StateMessage& state) {
  std::vector<std::string> encoded_state;
  encoded_state.emplace_back(clproto::encode(state.ee_state));
  encoded_state.emplace_back(clproto::encode(state.joint_state));
  encoded_state.emplace_back(clproto::encode(state.jacobian));
  encoded_state.emplace_back(clproto::encode(state.mass));
  encoded_state.emplace_back(clproto::encode(state.external_wrench));
  encoded_state.emplace_back(clproto::encode(state.external_torque));
  return encoded_state;
}

/**
 * @brief Encode a command message into the serialized binary format.
 * @param state The CommandMessage to encode
 * @return An ordered vector of encoded strings representing the command message fields
 */
inline std::vector<std::string> encode_command(CommandMessage& command) {
  if (command.control_type.size() != command.joint_state.get_size()) {
    if (command.control_type.size() == 1 && !command.joint_state.is_empty()) {
      command.control_type = std::vector<int>(command.joint_state.get_size(), command.control_type.at(0));
    } else {
      throw network_interfaces::zmq::exceptions::InvalidControlTypeVectorException(
          "The size of field 'control_type' of the CommandMessage does not correspond "
          "to the size of the field 'joint state'."
      );
    }
  }
  for (std::size_t i = 0; i < command.joint_state.get_size(); ++i) {
    if (static_cast<network_interfaces::control_type_t>(command.control_type.at(i))
        >= network_interfaces::control_type_t::END) {
      throw network_interfaces::zmq::exceptions::UnknownControlTypeException(
          "The desired 'control_type' of the CommandMessage is unknown."
      );
    }
  }
  std::vector<std::string> encoded_command;
  encoded_command.emplace_back(
      clproto::encode(state_representation::Parameter<std::vector<int>>("control_type", command.control_type)));
  encoded_command.emplace_back(clproto::encode(command.joint_state));
  return encoded_command;
}

// --- Decoding methods --- //

/**
 * @brief Decode a state message from the serialized binary format.
 * @param message An ordered vector of encoded strings representing the state message fields
 * @return The equivalent StateMessage
 */
inline StateMessage decode_state(const std::vector<std::string>& message) {
  StateMessage state;
  state.ee_state = clproto::decode<state_representation::CartesianState>(message.at(0));
  state.joint_state = clproto::decode<state_representation::JointState>(message.at(1));
  state.jacobian = clproto::decode<state_representation::Jacobian>(message.at(2));
  state.mass = clproto::decode<state_representation::Parameter<Eigen::MatrixXd>>(message.at(3));
  state.external_wrench = clproto::decode<state_representation::CartesianWrench>(message.at(4));
  state.external_torque = clproto::decode<state_representation::Parameter<Eigen::MatrixXd>>(message.at(5));
  return state;
}

/**
 * @brief Decode a command message from the serialized binary format.
 * @param message An ordered vector of encoded strings representing the command message fields
 * @return The equivalent CommandMessage
 */
inline CommandMessage decode_command(const std::vector<std::string>& message) {
  CommandMessage command;
  command.control_type = clproto::decode<state_representation::Parameter<std::vector<int>>>(message.at(0)).get_value();
  command.joint_state = clproto::decode<state_representation::JointState>(message.at(1));
  return command;
}

// --- Configuration methods --- //

/**
 * @brief Configure the zmq subscriber socket to receive messages.
 * @param context The ZMQ context object
 * @param[in,out] subscriber The subscription socket that is used to receive messages
 * @param subscriber_uri The URI (IP:Port) of the network socket
 * @param bind_subscriber Optional flag to decide if subscriber should bind (true) or connect (false)
 */
inline void configure_subscriber(
    ::zmq::context_t& context, ::zmq::socket_t& subscriber, const std::string& subscriber_uri,
    bool bind_subscriber = false
) {
  subscriber = ::zmq::socket_t(context, ZMQ_SUB);
  subscriber.set(::zmq::sockopt::conflate, 1);
  subscriber.set(::zmq::sockopt::subscribe, "");
  if (bind_subscriber) {
    subscriber.bind("tcp://" + subscriber_uri);
  } else {
    subscriber.connect("tcp://" + subscriber_uri);
  }
}

/**
 * @brief Configure the zmq subscriber socket to publish messages.
 * @param context The ZMQ context object
 * @param[in,out] publisher The publication socket that is used to send messages
 * @param publisher_uri The URI (IP:Port) of the network socket
 * @param bind_publisher Optional flag to decide if publisher should bind (true) or connect (false)
 */
inline void configure_publisher(
    ::zmq::context_t& context, ::zmq::socket_t& publisher, const std::string& publisher_uri, bool bind_publisher = true
) {
  publisher = ::zmq::socket_t(context, ZMQ_PUB);
  if (bind_publisher) {
    publisher.bind("tcp://" + publisher_uri);
  } else {
    publisher.connect("tcp://" + publisher_uri);
  }
}

/**
 * @brief Configure the zmq sockets to publish and receive messages.
 * @param context The ZMQ context object
 * @param[in,out] subscriber The subscription socket that is used to receive messages
 * @param subscriber_uri The URI (IP:Port) of the subscribing network socket
 * @param[in,out] publisher The publication socket that is used to send messages
 * @param publisher_uri The URI (IP:Port) of the publishing network socket
 * @param bind_subscriber Optional flag to decide if subscriber should bind (true) or connect (false)
 * @param bind_publisher Optional flag to decide if publisher should bind (true) or connect (false)
 */
inline void configure_sockets(
    ::zmq::context_t& context, ::zmq::socket_t& subscriber, const std::string& subscriber_uri,
    ::zmq::socket_t& publisher, const std::string& publisher_uri, bool bind_subscriber = false,
    bool bind_publisher = true
) {
  configure_subscriber(context, subscriber, subscriber_uri, bind_subscriber);
  configure_publisher(context, publisher, publisher_uri, bind_publisher);
}

// --- Transceiving methods --- //

/**
 * @brief Send a sequence of encoded field messages.
 * @param fields An ordered vector of encoded fields
 * @param publisher The configured ZMQ publisher socket
 * @return True if the state was published, false otherwise
 */
inline bool send(const std::vector<std::string>& fields, ::zmq::socket_t& publisher) {
  ::zmq::message_t message(fields.size() * CLPROTO_PACKING_MAX_FIELD_LENGTH);
  clproto::pack_fields(fields, static_cast<char*>(message.data()));
  auto res = publisher.send(message, ::zmq::send_flags::none);
  return res.has_value();
}

/**
 * @brief Send a state message.
 * @param state The StateMessage to publish
 * @param publisher The configured ZMQ publisher socket
 * @return True if the state was published, false otherwise
 */
inline bool send(const StateMessage& state, ::zmq::socket_t& publisher) {
  return send(encode_state(state), publisher);
}

/**
 * @brief Send a command message.
 * @param command The CommandMessage to publish
 * @param publisher The configured ZMQ publisher socket
 * @return True if the command was published, false otherwise
 */
inline bool send(CommandMessage& command, ::zmq::socket_t& publisher) {
  return send(encode_command(command), publisher);
}

/**
 * @brief Receive a sequence of encoded field messages from the socket.
 * @param[out] fields A vector of encoded fields to modify by reference if a message is available
 * @param subscriber The configured ZMQ subscriber socket
 * @param wait Optional flag to decide if the subscriber should wait for a message (true) or continue (false)
 * @return True if a state was received, false otherwise
 */
inline bool receive(std::vector<std::string>& fields, ::zmq::socket_t& subscriber, bool wait = false) {
  ::zmq::recv_flags recv_flag = wait ? ::zmq::recv_flags::none : ::zmq::recv_flags::dontwait;
  ::zmq::message_t message;
  auto res = subscriber.recv(message, recv_flag);
  if (res) {
    fields = clproto::unpack_fields(static_cast<const char*>(message.data()));
  }
  return res.has_value();
}

/**
 * @brief Receive a state message from the socket.
 * @param[out] state The StateMessage object to modify by reference if a message is available
 * @param subscriber The configured ZMQ subscriber socket
 * @param wait Optional flag to decide if the subscriber should wait for a message (true) or continue (false)
 * @return True if a state was received, false otherwise
 */
inline bool receive(StateMessage& state, ::zmq::socket_t& subscriber, bool wait = false) {
  std::vector<std::string> fields;
  auto res = receive(fields, subscriber, wait);
  if (res) {
    state = decode_state(fields);
  }
  return res;
}

/**
 * @brief Receive a command message from the socket.
 * @param[out] command The CommandMessage object to modify by reference if a message is available
 * @param subscriber The configured ZMQ subscriber socket
 * @param wait Optional flag to decide if the subscriber should wait for a message (true) or continue (false)
 * @return True if a command was received, false otherwise
 */
inline bool receive(CommandMessage& command, ::zmq::socket_t& subscriber, bool wait = false) {
  std::vector<std::string> fields;
  auto res = receive(fields, subscriber, wait);
  if (res) {
    command = decode_command(fields);
  }
  return res;
}
}// namespace network_interfaces::zmq
