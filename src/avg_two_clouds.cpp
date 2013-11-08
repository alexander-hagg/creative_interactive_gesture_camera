#include <iostream>
//ros includes
#include <ros/ros.h>
#include <ros/console.h>
#include <sensor_msgs/PointCloud2.h>
//point cloud includes
#include <pcl_ros/point_cloud.h>
#include <pcl_ros/io/pcd_io.h>
#include <pcl/registration/icp.h>
//pcl filter includes
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/approximate_voxel_grid.h>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/statistical_outlier_removal.h>

typedef pcl::PointCloud<pcl::PointXYZRGB> PointCloud;
ros::Publisher pub;
int counter;
pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_target (new pcl::PointCloud<pcl::PointXYZRGB>);
pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_new (new pcl::PointCloud<pcl::PointXYZRGB>);

int numFrames;

void callback(const PointCloud::ConstPtr& cloud){
  if(counter == 0)
  {
    *cloud_target = *cloud;
    //increment counter
    counter++;
    numFrames++;
  }else if(counter < 4) 
  {  
    *cloud_new = *cloud;
    //Loop through points
    int h,w;
    if(cloud_new->height * cloud_new->width < cloud_target->height * cloud_target->width)
    {
      h = cloud_new->height;
      w = cloud_new->width;
    }
    else
    {
     h = cloud_target->height;
     w = cloud_target->width;
    }
    int count_pcl = 0;
    float avgX = 0;
    float avgY = 0;
    float avgZ = 0;
    
    for(int i = 0; i < h; i++){
      for(int j = 0; j < w; j++){
        count_pcl++;
        avgX = (cloud_target->points[count_pcl].x * numFrames + cloud_new->points[count_pcl].x)/(numFrames+1);
        avgY = (cloud_target->points[count_pcl].y * numFrames + cloud_new->points[count_pcl].y)/(numFrames+1);
        avgZ = (cloud_target->points[count_pcl].z * numFrames + cloud_new->points[count_pcl].z)/(numFrames+1);
        cloud_target->points[count_pcl].x = avgX;
        cloud_target->points[count_pcl].y = avgY;
        //cloud_target->points[count_pcl].z = avgZ;
      }
    }
    numFrames++;
    counter++;
    // Saving transformed input cloud.
    if(counter == 4){
      ROS_INFO("Saving Collection of Data");
      //pcl::io::savePCDFileASCII ("transformed_cloud.pcd", *cloud_target);
      counter = 0;
      numFrames == 0;
      pub.publish(cloud_target);
    }
  }
}//end callback

int main (int argc, char** argv)
{
  counter = 0;
  numFrames = 0;
  ros::init(argc, argv, "pcl_avg");
  ros::NodeHandle nh;
  ros::Subscriber sub = nh.subscribe<PointCloud>("points2", 1, callback);
  
  pub = nh.advertise<PointCloud> ("avg_filtered_points2", 1);
  ros::spin();
  return (0);
}
