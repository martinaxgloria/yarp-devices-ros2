/*
 * SPDX-FileCopyrightText: 2006-2021 Istituto Italiano di Tecnologia (IIT)
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "Odometry2D_nws_ros2.h"
#include <yarp/os/LogComponent.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Stamp.h>
#include <math.h>
#include <Ros2Utils.h>
#include <yarp/dev/OdometryData.h>

YARP_LOG_COMPONENT(ODOMETRY2D_NWS_ROS2, "yarp.devices.Odometry2D_nws_ros2")

Odometry2D_nws_ros2::Odometry2D_nws_ros2() : yarp::os::PeriodicThread(DEFAULT_THREAD_PERIOD)
{
}

Odometry2D_nws_ros2::~Odometry2D_nws_ros2()
{
    m_odometry2D_interface = nullptr;
}


bool Odometry2D_nws_ros2::attach(yarp::dev::PolyDriver* driver)
{

    if (driver->isValid())
    {
        driver->view(m_odometry2D_interface);
    } else {
        yCError(ODOMETRY2D_NWS_ROS2) << "not valid driver";
    }

    if (m_odometry2D_interface == nullptr)
    {
        yCError(ODOMETRY2D_NWS_ROS2, "Subdevice passed to attach method is invalid");
        return false;
    }
    PeriodicThread::setPeriod(m_period);
    return PeriodicThread::start();
}


bool Odometry2D_nws_ros2::detach()
{
    if (PeriodicThread::isRunning())
    {
        PeriodicThread::stop();
    }
    m_odometry2D_interface = nullptr;
    return true;
}

bool Odometry2D_nws_ros2::threadInit()
{
    return true;
}

bool Odometry2D_nws_ros2::open(yarp::os::Searchable &config)
{
    if (!config.check("period")) {
        yCWarning(ODOMETRY2D_NWS_ROS2) << "missing 'period' parameter, using default value of" << DEFAULT_THREAD_PERIOD;
    } else {
        m_period = config.find("period").asFloat64();
    }

    if (!config.check("node_name")) {
        yCError(ODOMETRY2D_NWS_ROS2) << "missing node_name parameter";
        return false;
    }
    m_nodeName = config.find("node_name").asString();
    if (m_nodeName[0] == '/') {
        yCError(ODOMETRY2D_NWS_ROS2) << "node_name parameter cannot begin with '/'";
        return false;
    }

    if (!config.check("topic_name")) {
        yCError(ODOMETRY2D_NWS_ROS2) << "missing topic_name parameter";
        return false;
    }
    m_topicName = config.find("topic_name").asString();
    if (m_topicName[0] != '/') {
        yCError(ODOMETRY2D_NWS_ROS2) << "missing initial / in topic_name parameter";
        return false;
    }

    if (!config.check("odom_frame")) {
        yCError(ODOMETRY2D_NWS_ROS2) << "missing odom_frame parameter";
        return false;
    }
    m_odomFrame = config.find("odom_frame").asString();


    if (!config.check("base_frame")) {
        yCError(ODOMETRY2D_NWS_ROS2) << "missing base_frame parameter";
        return false;
    }
    m_baseFrame = config.find("base_frame").asString();

    if (config.check("subdevice")) {
        yarp::os::Property p;
        p.fromString(config.toString(), false);
        p.put("device", config.find("subdevice").asString());

        if (!m_driver.open(p) || !m_driver.isValid()) {
            yCError(ODOMETRY2D_NWS_ROS2) << "failed to open subdevice.. check params";
            return false;
        }

        if (!attach(&m_driver)) {
            yCError(ODOMETRY2D_NWS_ROS2) << "failed to open subdevice.. check params";
            return false;
        }
    }

    m_node = NodeCreator::createNode(m_nodeName);
    if (m_node == nullptr) {
        yCError(ODOMETRY2D_NWS_ROS2) << " opening " << m_nodeName << " Node, check your yarp-ROS2 network configuration\n";
        return false;
    }

    ros2Publisher_odometry = m_node->create_publisher<nav_msgs::msg::Odometry>(m_topicName, 10);
    return true;
}

void Odometry2D_nws_ros2::threadRelease()
{
}

void Odometry2D_nws_ros2::run()
{

    if (m_odometry2D_interface!=nullptr && ros2Publisher_odometry) {
        yarp::os::Stamp timeStamp(static_cast<int>(m_stampCount++), yarp::os::Time::now());
        yarp::dev::OdometryData odometryData;
        m_odometry2D_interface->getOdometry(odometryData);
        nav_msgs::msg::Odometry odometryMsg;
        odometryMsg.header.frame_id = m_odomFrame;

        odometryMsg.header.stamp.sec = int(timeStamp.getTime());
        odometryMsg.header.stamp.nanosec = int(1000000 * (timeStamp.getTime() - int(timeStamp.getTime())));
        odometryMsg.child_frame_id = m_baseFrame;

        odometryMsg.pose.pose.position.x = odometryData.odom_x;
        odometryMsg.pose.pose.position.y = odometryData.odom_y;
        odometryMsg.pose.pose.position.z = 0.0;
        geometry_msgs::msg::Quaternion odom_quat;
        double halfYaw = odometryData.odom_theta * DEG2RAD * 0.5;
        double cosYaw = cos(halfYaw);
        double sinYaw = sin(halfYaw);
        odom_quat.x = 0;
        odom_quat.y = 0;
        odom_quat.z = sinYaw;
        odom_quat.w = cosYaw;
        odometryMsg.pose.pose.orientation = odom_quat;
        odometryMsg.twist.twist.linear.x = odometryData.base_vel_x;
        odometryMsg.twist.twist.linear.y = odometryData.base_vel_y;
        odometryMsg.twist.twist.linear.z = 0;
        odometryMsg.twist.twist.angular.x = 0;
        odometryMsg.twist.twist.angular.y = 0;
        odometryMsg.twist.twist.angular.z = odometryData.base_vel_theta * DEG2RAD;

        ros2Publisher_odometry->publish(odometryMsg);

    } else{
        yCError(ODOMETRY2D_NWS_ROS2) << "the interface is not valid";
    }
}

bool Odometry2D_nws_ros2::close()
{
    yCTrace(ODOMETRY2D_NWS_ROS2);
    if (PeriodicThread::isRunning())
    {
        PeriodicThread::stop();
    }

    detach();
    return true;
}