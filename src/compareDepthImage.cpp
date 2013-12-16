#include <iostream>
//ros includes
#include <ros/ros.h>
#include <ros/console.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/Image.h>
#include <creative_interactive_gesture_camera/floatTest.h>
//point cloud includes
#include <pcl_ros/point_cloud.h>
#include <pcl_ros/io/pcd_io.h>
#include <pcl/registration/icp.h>


ros::Publisher pub;
int counter;
int saveCounter;

void callback(const sensor_msgs::ImageConstPtr& image)
{
  //Want to determine if the depth image is being rounded somehow or if the values in an area are all the same
  int image_size = image->height * image->width; //size should be height*width as image is single channel
  creative_interactive_gesture_camera::floatTest msg; //create floatTest message type
  int checkIndex = rand()%image_size; //create some random index to check 
  float checkValue = image->data[checkIndex]; //pull out value
  msg.header = image->header; //want to check header values
  msg.data = image->data.size()/sizeof(float); //store in float data to be looked at
  
  pub.publish(msg); //publish msg data to roscore
  

}//end callback

int main (int argc, char** argv)
{
  ROS_INFO("Starting Depth Image Test Node");
  counter = 0;
  saveCounter = 0;
  ros::init(argc, argv, "depth_image_test");
  ros::NodeHandle nh;
  image_transport::ImageTransport it(nh);
  image_transport::Subscriber sub = it.subscribe("depth_image", 1, callback);
  pub = nh.advertise<creative_interactive_gesture_camera::floatTest> ("depth_image_info", 1);
  
  ros::spin();
  return (0);
}
