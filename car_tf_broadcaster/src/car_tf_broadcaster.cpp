
#include "car_tf_broadcaster.h"
geometry_msgs::Pose CarTF::transformPose(const geometry_msgs::Pose &pose,
                                         const geometry_msgs::TransformStamped &transform)
{
    geometry_msgs::PoseStamped transformed_pose;
    geometry_msgs::PoseStamped orig_pose;
    orig_pose.pose = pose;
    tf2::doTransform(orig_pose, transformed_pose, transform);

    return transformed_pose.pose;
}

geometry_msgs::TransformStamped CarTF::getTransform(const std::string &target,
                                                    const std::string &source)
{
    geometry_msgs::TransformStamped tf;
    try
    {
        // _tf_buffer->setUsingDedicatedThread(true);
        tf = _tf_buffer->lookupTransform(target, source, ros::Time(0), ros::Duration(1));
    }
    catch (const tf2::LookupException &ex)
    {
        ROS_ERROR("%s", ex.what());
    }
    return tf;
}

void CarTF::callbackGnssPose(const geometry_msgs::PoseStampedConstPtr &msg)
{
    _gnss_pose = *msg;
}

void CarTF::callbackGoalPose(const geometry_msgs::PoseStamped &msg)
{
    _goal_pose_stamped = msg;
    _goal_pose_flag = true;
}

void CarTF::callbackCostMap(const nav_msgs::OccupancyGrid &msg)
{
    _costmap_frame_id = msg.header.frame_id;
    _costmap_flag = true;
}

void CarTF::callbackRvizStartPose(const geometry_msgs::PoseWithCovarianceStamped &msg)
{
    _rivz_start_pose = msg;
}

void CarTF::callbackTimerPublishTF(const ros::TimerEvent &e)
{
    publishTF();
}

void CarTF::publishTF()
{
    // map->gps
    geometry_msgs::TransformStamped gps_transform;

    gps_transform.header.frame_id = map_frame;
    gps_transform.header.stamp = ros::Time::now();

    gps_transform.child_frame_id = gps_frame;

    geometry_msgs::Pose gnss_pose;
    if (use_rviz_start)
    {
        gnss_pose = _rivz_start_pose.pose.pose;
    }
    else
    {
        gnss_pose = _gnss_pose.pose;
    }
    gps_transform.transform.translation.x = gnss_pose.position.x;
    gps_transform.transform.translation.y = gnss_pose.position.y;
    gps_transform.transform.translation.z = gnss_pose.position.z;
    gps_transform.transform.rotation = gnss_pose.orientation;

    _tf_broadcaster.sendTransform(gps_transform);

    // gps->base_link
    geometry_msgs::TransformStamped base_link_transform;

    base_link_transform.header.frame_id = gps_frame;
    base_link_transform.header.stamp = ros::Time::now();

    base_link_transform.child_frame_id = base_link_frame;

    base_link_transform.transform.translation.x = base_link_trans_x;
    base_link_transform.transform.translation.y = base_link_trans_y;
    base_link_transform.transform.translation.z = base_link_trans_z;

    geometry_msgs::Quaternion q_gps_base_link;
    q_gps_base_link = tf::createQuaternionMsgFromRollPitchYaw(base_link_rotation_roll,
                                                base_link_rotation_pitch,
                                                base_link_rotation_yaw);
    base_link_transform.transform.rotation.w = q_gps_base_link.w;
    base_link_transform.transform.rotation.x = q_gps_base_link.x;
    base_link_transform.transform.rotation.y = q_gps_base_link.y;
    base_link_transform.transform.rotation.z = q_gps_base_link.z;

    _tf_broadcaster.sendTransform(base_link_transform);

    // base_link->lidar_rs
    geometry_msgs::TransformStamped lidar_transform;

    lidar_transform.header.frame_id = base_link_frame;
    lidar_transform.header.stamp = ros::Time::now();

    lidar_transform.child_frame_id = lidar_frame;

    lidar_transform.transform.translation.x = lidar_trans_x;
    lidar_transform.transform.translation.y = lidar_trans_y;
    lidar_transform.transform.translation.z = lidar_trans_z;

    geometry_msgs::Quaternion q_base_link_lidar_rs;
    q_base_link_lidar_rs = tf::createQuaternionMsgFromRollPitchYaw(lidar_rotation_roll,
                                                lidar_rotation_pitch,
                                                lidar_rotation_yaw);
    lidar_transform.transform.rotation.w = q_base_link_lidar_rs.w;
    lidar_transform.transform.rotation.x = q_base_link_lidar_rs.x;
    lidar_transform.transform.rotation.y = q_base_link_lidar_rs.y;
    lidar_transform.transform.rotation.z = q_base_link_lidar_rs.z;

    _tf_broadcaster.sendTransform(lidar_transform);

    if (_costmap_flag && _goal_pose_flag)
    {
        geometry_msgs::TransformStamped broad_target_tf;

        broad_target_tf.header.frame_id = "map";
        broad_target_tf.header.stamp = ros::Time::now();

        broad_target_tf.child_frame_id = "target_car";

        geometry_msgs::Pose goal_pose_in_map_frame;
        _target_tf = getTransform("map", _goal_pose_stamped.header.frame_id);
        goal_pose_in_map_frame = transformPose(_goal_pose_stamped.pose, _target_tf);

        broad_target_tf.transform.translation.x = goal_pose_in_map_frame.position.x;
        broad_target_tf.transform.translation.y = goal_pose_in_map_frame.position.y;
        broad_target_tf.transform.translation.z = goal_pose_in_map_frame.position.z;
        broad_target_tf.transform.rotation = goal_pose_in_map_frame.orientation;

        _tf_broadcaster.sendTransform(broad_target_tf);
    }

    //广播结束
}

CarTF::CarTF() : _nh(""), _private_nh("~")
{

    _private_nh.param<double>("lidar_trans_x", lidar_trans_x, 4.34);
    _private_nh.param<double>("lidar_trans_y", lidar_trans_y, 0.58);
    _private_nh.param<double>("lidar_trans_z", lidar_trans_z, 0.0);
    _private_nh.param<double>("lidar_rotation_roll", lidar_rotation_roll, 0.0);
    _private_nh.param<double>("lidar_rotation_pitch", lidar_rotation_pitch, 0.0);
    _private_nh.param<double>("lidar_rotation_yaw", lidar_rotation_yaw, 0.0);
    _private_nh.param<double>("base_link_trans_x", base_link_trans_x, 0.0);
    _private_nh.param<double>("base_link_trans_y", base_link_trans_y, 0.0);
    _private_nh.param<double>("base_link_trans_z", base_link_trans_z, 0.0);
    _private_nh.param<double>("base_link_rotation_roll", base_link_rotation_roll, 0.0);
    _private_nh.param<double>("base_link_rotation_pitch", base_link_rotation_pitch, 0.0);
    _private_nh.param<double>("base_link_rotation_yaw", base_link_rotation_yaw, 0.0);
    _private_nh.param<std::string>("map_frame", map_frame, "map");
    _private_nh.param<std::string>("gps_frame", gps_frame, "gps");
    _private_nh.param<std::string>("base_link_frame", base_link_frame, "base_link");
    _private_nh.param<std::string>("lidar_frame_id", lidar_frame, "rslidar");
    _private_nh.param<std::string>("pose_topic", pose_topic, "gnss_pose");
    _private_nh.param<bool>("use_rviz_start", use_rviz_start, false);

    _sub_gnss_pose = _nh.subscribe(pose_topic, 1, &CarTF::callbackGnssPose, this);
    _sub_rviz_start_pose = _nh.subscribe("initialpose", 1, &CarTF::callbackRvizStartPose, this);
    _sub_goal_pose = _nh.subscribe("move_base_simple/goal", 1, &CarTF::callbackGoalPose, this);
    _sub_cost_map = _nh.subscribe("global_cost_map", 1, &CarTF::callbackCostMap, this);

    _timer_tf = _nh.createTimer(ros::Duration(0.02), &CarTF::callbackTimerPublishTF, this);

    _tf_buffer = std::make_shared<tf2_ros::Buffer>();
    _tf_listener = std::make_shared<tf2_ros::TransformListener>(*_tf_buffer);

    _rivz_start_pose.pose.pose.orientation.w = 1.0;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "car_tf_broadcaster");

    CarTF obj;

    ros::spin();

    return 0;
}