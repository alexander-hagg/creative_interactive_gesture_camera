#include <stdio.h>
#include <vector>
#include <exception>
#include <iostream>
#include <fstream>

//ros include files
#include <image_transport/image_transport.h>
#include <ros/ros.h>
#include <std_msgs/Int32.h>
#include <sensor_msgs/CameraInfo.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud_conversion.h>

#include <pcl_ros/io/pcd_io.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/features/fpfh_omp.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/registration/sample_consensus_prerejective.h>
#include <pcl/registration/icp.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/keypoints/sift_keypoint.h>

#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/time_synchronizer.h>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/core/core.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <Eigen/Core>

using namespace message_filters;
namespace enc = sensor_msgs::image_encodings;
using namespace std;

typedef pcl::PointCloud<pcl::PointXYZRGB> PointCloud;

PointCloud::Ptr cloud1 (new pcl::PointCloud<pcl::PointXYZRGB>);
PointCloud::Ptr cloud2 (new pcl::PointCloud<pcl::PointXYZRGB>);
PointCloud::Ptr object_aligned (new pcl::PointCloud<pcl::PointXYZRGB>);
ros::Publisher pub_cloudRGB;

vector<cv::KeyPoint> keypoints1;
vector<cv::KeyPoint> keypoints2;
cv::Mat descriptors1;
cv::Mat descriptors2;

pcl::PointCloud<pcl::PointWithScale>::Ptr keypoints_out1 (new pcl::PointCloud<pcl::PointWithScale>);
pcl::PointCloud<pcl::PointWithScale>::Ptr keypoints_out2 (new pcl::PointCloud<pcl::PointWithScale>);
//sift detection
pcl::SIFTKeypoint<pcl::PointXYZRGB, pcl::PointWithScale>::Ptr sift_detect (new pcl::SIFTKeypoint<pcl::PointXYZRGB, pcl::PointWithScale>);
//Determines transform to align two point clouds
//pcl::SampleConsensusPrerejective<pcl::PointXYZRGB, pcl::PointXYZRGB, pcl::PointWithScale>::Ptr 
  //align (new pcl::SampleConsensusPrerejective<pcl::PointXYZRGB, pcl::PointXYZRGB, pcl::PointWithScale>);
//Use FLANN-based KdTree to perform neighborhood searches
pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZRGB>());

int counter;
//, const sensor_msgs::CameraInfoConstPtr& cam_info
void findKeyPoints(const sensor_msgs::ImageConstPtr& image, const sensor_msgs::PointCloud2ConstPtr& input_cloud)
{
  ROS_INFO("Inside findKeyPoints");
  cv_bridge::CvImageConstPtr cv_ptr;

  cv::SurfFeatureDetector detector(400); //using SURF detection
  cv::SurfDescriptorExtractor extractor;
  cv_ptr = cv_bridge::toCvShare(image, enc::BGR8);
  
  sift_detect->setSearchMethod(tree);
  sift_detect->setScales(0.025,4,5);
  sift_detect->setMinimumContrast(0.005f);
  
  if(counter == 0)
  {
    pcl::fromROSMsg(*input_cloud,*cloud1);
    sift_detect->setInputCloud(cloud1);
    sift_detect->compute(*keypoints_out1);
    
    //Look at image for keypoints, convert to open cv ptr, data stored in image
    detector.detect(cv_ptr->image,keypoints1);
    extractor.compute(cv_ptr->image, keypoints1, descriptors1);
    counter++;
  } else
  {
    pcl::fromROSMsg(*input_cloud,*cloud2);
    sift_detect->setInputCloud(cloud2);
    sift_detect->compute(*keypoints_out2);
    ROS_INFO("Number of KeyPoints2: %d", keypoints_out2->points.size());
    //Find transform between last image and current image
    detector.detect(cv_ptr->image,keypoints2);
    extractor.compute(cv_ptr->image, keypoints2, descriptors2);
    //aligning pointclouds
    pcl::SampleConsensusPrerejective<pcl::PointXYZRGB, pcl::PointXYZRGB, pcl::PointWithScale> align;
    align.setInputSource (cloud2);
    align.setSourceFeatures (keypoints_out2);
    align.setInputTarget (cloud1);
    align.setTargetFeatures (keypoints_out1);
    align.setNumberOfSamples (3); // Number of points to sample for generating/prerejecting a pose
    align.setCorrespondenceRandomness (2); // Number of nearest features to use
    align.setSimilarityThreshold (0.6f); // Polygonal edge length similarity threshold
    align.setMaxCorrespondenceDistance (1.5f * 0.005f); // Set inlier threshold
    align.setInlierFraction (0.25f); // Set required inlier fraction
    align.align (*object_aligned);
    if (align.hasConverged ())
    {
      Eigen::Matrix4f transformation = align.getFinalTransformation ();
      ROS_INFO ("    | %6.3f %6.3f %6.3f | \n", transformation (0,0), transformation (0,1), transformation (0,2));
      ROS_INFO ("R = | %6.3f %6.3f %6.3f | \n", transformation (1,0), transformation (1,1), transformation (1,2));
      ROS_INFO ("    | %6.3f %6.3f %6.3f | \n", transformation (2,0), transformation (2,1), transformation (2,2));
      ROS_INFO ("\n");
      ROS_INFO ("t = < %0.3f, %0.3f, %0.3f >\n", transformation (0,3), transformation (1,3), transformation (2,3));
      ROS_INFO ("\n");
      ROS_INFO ("Inliers: %i\n", align.getInliers().size());
    }
    else
    {
      ROS_INFO("Alignment failed!");
    } 
    counter = 0;
  }
  //pub_cloudRGB.publish(cloudRGB_);
  //delete cv_ptr;
}
/*----------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
  counter = 0;
  //initialize ros
  ros::init (argc, argv, "find_keypoints");
  ros::NodeHandle nh;
  ROS_INFO("Starting Camera sync");
  //initialize image transport object
  image_transport::ImageTransport it(nh);
 
  //create cameraSync object relating to current ros handle
  //cameraSync cs(nh);
  //Adding in syncing subscribers
  message_filters::Subscriber<sensor_msgs::Image> image_sub(nh, "rgb_data", 1);
  message_filters::Subscriber<sensor_msgs::PointCloud2> pc_sub(nh, "transform_points2", 1);
  //message_filters::Subscriber<sensor_msgs::CameraInfo> ci_sub(nh, "camera_info",1);
  
  typedef sync_policies::ApproximateTime<sensor_msgs::Image, sensor_msgs::PointCloud2> MySyncPolicy;//, sensor_msgs::CameraInfo
  
  message_filters::Synchronizer<MySyncPolicy> sync(MySyncPolicy(100), image_sub, pc_sub);//, ci_sub
  sync.registerCallback(boost::bind(&findKeyPoints, _1, _2));
  //end syncing subscriber  
  //initialize publishers
  pub_cloudRGB = nh.advertise<PointCloud > ("keypoints",1);
  //loop while ros core is operational or Ctrl-C is used
  while(ros::ok()){
    ros::spin();
  }
    
  return 0;
}
