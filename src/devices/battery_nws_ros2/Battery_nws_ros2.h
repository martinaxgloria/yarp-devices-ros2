/*
 * SPDX-FileCopyrightText: 2006-2022 Istituto Italiano di Tecnologia (IIT)
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */


#ifndef YARP_ROS2_BATTERY_NWS_ROS2_H
#define YARP_ROS2_BATTERY_NWS_ROS2_H
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/battery_state.hpp>

#include <yarp/dev/IBattery.h>
#include <yarp/os/PeriodicThread.h>
#include <yarp/os/Stamp.h>
#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/WrapperSingle.h>

#define DEFAULT_THREAD_PERIOD 0.02 //s

/**
 * @ingroup dev_impl_nws_ros2
 *
 * \section Battery_nws_ros2_parameters Device description
 * \brief `Battery_nws_ros2`: A ros2 nws to get the status of a battery and publish it on a ros2 topic.
 * The attached device must implement a `yarp::dev::IBattery` interface.
 *
 * Parameters required by this device are:
 * | Parameter name      | SubParameter            | Type    | Units          | Default Value | Required                       | Description                                             | Notes |
 * |:-------------------:|:-----------------------:|:-------:|:--------------:|:-------------:|:-----------------------------: |:-------------------------------------------------------:|:-----:|
 * | period              |      -                  | double  | s              |   0.02        | No                             | refresh period of the broadcasted values in s           | default 0.02s |
 * | node_name           |      -                  | string  | -              |   -           | Yes                            | name of the ros2 node                                   |      |
 * | topic_name          |      -                  | string  | -              |   -           | Yes                            | name of the topic where the device must publish the data| must begin with an initial '/'     |
 * | subdevice           |      -                  | string  | -              |   -           | alternative to 'attach' action | name of the subdevice to use as a data source           | when used, parameters for the subdevice must be provided as well |
 *
 */


class Battery_nws_ros2 :
        public yarp::os::PeriodicThread,
        public yarp::dev::DeviceDriver,
        public yarp::dev::WrapperSingle
{
public:
    Battery_nws_ros2();
    ~Battery_nws_ros2();

    // DeviceDriver
    bool open(yarp::os::Searchable &params) override;
    bool close() override;

    // WrapperSingle
    bool attach(yarp::dev::PolyDriver* driver) override;
    bool detach() override;

    // PeriodicThread
    bool threadInit() override;
    void threadRelease() override;
    void run() override;


private:
    // parameters from configuration
    std::string m_topicName;
    std::string m_nodeName;

    // stamp count for timestamp
    yarp::os::Stamp m_timeStamp;

    // period for thread
    double m_period{DEFAULT_THREAD_PERIOD};

    //ros2 node
    sensor_msgs::msg::BatteryState  battMsg;
    rclcpp::Node::SharedPtr m_node;
    rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr m_ros2Publisher =nullptr;

    //interfaces
    yarp::dev::PolyDriver m_driver;
    yarp::dev::IBattery *m_battery_interface{nullptr};
};

#endif // YARP_ROS2_BATTERY_NWS_ROS2_H
