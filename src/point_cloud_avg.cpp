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

#include <creative_interactive_gesture_camera/Mapping2D.h> //Custom message for passing corresponding 2D points for each 3D point
#include <creative_interactive_gesture_camera/Point2D.h>//Custom message for passing corresponding 2D points for each 3D point

typedef pcl::PointCloud<pcl::PointXYZRGB> PointCloud;
ros::Publisher pub;
int counter;
int saveCounter;
pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_target (new pcl::PointCloud<pcl::PointXYZRGB>);
pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_new (new pcl::PointCloud<pcl::PointXYZRGB>);

void callback(const PointCloud::ConstPtr& cloud){
  if(counter == 0)
  {
    ROS_INFO("Obtained the point cloud for filtering");
    *cloud_target = *cloud;
    //passthrough filter taking only point between 0.15-1.0 meters
    pcl::PassThrough<pcl::PointXYZRGB> pass;
    pass.setInputCloud (cloud_target);
    pass.setFilterFieldName ("z");
    pass.setFilterLimits (0.15, 1.0);
    pass.filter (*cloud_target);
    //Removing noise, analyzing 50 neighboring points and removing 
    //outliers that are more than 1 std dev of the mean distance away from
    //the query point
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;
    sor.setInputCloud (cloud_target);
    sor.setMeanK (25); //number of neighbors to analyze for each point
    sor.setStddevMulThresh (1.0);
    sor.filter (*cloud_target);
    //increment counter
    counter++;
  }else if(counter != 0) 
  {  
    ROS_INFO("Running Filtering to get transform");
    *cloud_new = *cloud;
    //passthrough filtertaking only point between 0.15-1.0 meters
    pcl::PassThrough<pcl::PointXYZRGB> pass;
    pass.setInputCloud (cloud_new);
    pass.setFilterFieldName ("z");
    pass.setFilterLimits (0.10, 2.0);
    pass.filter (*cloud_new);
    //removing noise, as we did before
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;
    sor.setInputCloud (cloud_new);
    sor.setMeanK (25);
    sor.setStddevMulThresh (0.75);
    sor.filter (*cloud_new);
    cloud_new->header = cloud->header;
    //filter down incoming cloud
    /*pcl::PointCloud<pcl::PointXYZRGB>::Ptr filtered_cloud_target (new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr filtered_cloud_new (new pcl::PointCloud<pcl::PointXYZRGB>);
    
    pcl::ApproximateVoxelGrid<pcl::PointXYZRGB> approximate_voxel_filter;
    approximate_voxel_filter.setLeafSize (0.05f, 0.05f, 0.1f);
    
    approximate_voxel_filter.setInputCloud (cloud_target);
    approximate_voxel_filter.filter (*filtered_cloud_target);
    
    approximate_voxel_filter.setInputCloud (cloud_new);
    approximate_voxel_filter.filter (*filtered_cloud_new);
    
    //setup an icp object
    pcl::IterativeClosestPoint<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setInputSource(filtered_cloud_new);
    icp.setInputTarget(filtered_cloud_target);
    
    pcl::PointCloud<pcl::PointXYZRGB> Final;
    icp.align(Final);
    pcl::transformPointCloud (*cloud_new, Final, icp.getFinalTransformation ());
    *cloud_target += Final;*/
    counter++;
    // Saving transformed input cloud.
    if(counter == 2){
      //saveCounter++;
      //std::ostringstream saveName;
      //saveName << "transformed_cloud" << saveCounter << ".pcd";
      //std::string copyOfStr = saveName.str();
      //pcl::io::savePCDFileASCII (copyOfStr, *cloud_target);
      counter = 1;
      pub.publish(cloud_new);
    }
  }
}//end callback

int main (int argc, char** argv)
{
  ROS_INFO("Starting Filter Node");
  counter = 0;
  saveCounter = 0;
  ros::init(argc, argv, "pcl_filter");
  ros::NodeHandle nh;
  ros::Subscriber sub = nh.subscribe<pcl::PointCloud<pcl::PointXYZRGB> >("points2", 1, callback);
  
  pub = nh.advertise<pcl::PointCloud<pcl::PointXYZRGB> > ("transform_points2", 1);
  ros::spin();
  return (0);
}
