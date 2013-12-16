#include <PointCloudFilter.h>

PointCloudFilter::PointCloudFilter(ros::NodeHandle& nh) 
                : cloud_target(new pcl::PointCloud<pcl::PointXYZRGB>() ), 
                  cloud_new(new pcl::PointCloud<pcl::PointXYZRGB>() )
{
  nh_ = nh;
  counter = 0;
  saveCounter = 0;
  pub = nh_.advertise<pcl::PointCloud<pcl::PointXYZRGB> > ("transform_points2", 1);
  sub = nh_.subscribe<pcl::PointCloud<pcl::PointXYZRGB> > ("points2", 1, &PointCloudFilter::callback, &PointCloudFilter);
}

PointCloudFilter::~PointCloudFilter()
{
}

void PointCloudFilter::callback(const PointCloud::ConstPtr& cloud){
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
    sor.setMeanK (50);
    sor.setStddevMulThresh (1.0);
    sor.filter (*cloud_target);
    //increment counter
    counter++;
  }else if(counter < 4) 
  {  
    ROS_INFO("Running Filtering to get transform");
    *cloud_new = *cloud;
    //passthrough filtertaking only point between 0.15-1.0 meters
    pcl::PassThrough<pcl::PointXYZRGB> pass;
    pass.setInputCloud (cloud_new);
    pass.setFilterFieldName ("z");
    pass.setFilterLimits (0.15, 1.0);
    pass.filter (*cloud_new);
    //removing noise, as we did before
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;
    sor.setInputCloud (cloud_new);
    sor.setMeanK (50);
    sor.setStddevMulThresh (1.0);
    sor.filter (*cloud_new);
    //filter down incoming cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr filtered_cloud_target (new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr filtered_cloud_new (new pcl::PointCloud<pcl::PointXYZRGB>);
    
    pcl::ApproximateVoxelGrid<pcl::PointXYZRGB> approximate_voxel_filter;
    approximate_voxel_filter.setLeafSize (0.1f, 0.1f, 0.1f);
    
    approximate_voxel_filter.setInputCloud (cloud_target);
    approximate_voxel_filter.filter (*filtered_cloud_target);
    
    approximate_voxel_filter.setInputCloud (cloud_new);
    approximate_voxel_filter.filter (*filtered_cloud_new);
    
    //setup an icp object
    pcl::IterativeClosestPoint<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setInputCloud(filtered_cloud_new);
    icp.setInputTarget(filtered_cloud_target);
    
    pcl::PointCloud<pcl::PointXYZRGB> Final;
    icp.align(Final);
    pcl::transformPointCloud (*cloud_new, Final, icp.getFinalTransformation ());
    *cloud_target += Final;
    counter++;
    // Saving transformed input cloud.
    if(counter == 4){
      saveCounter++;
      /*std::ostringstream saveName;
      aveName << "transformed_cloud" << saveCounter << ".pcd";
      std::string copyOfStr = saveName.str();
      //pcl::io::savePCDFileASCII (copyOfStr, *cloud_target);*/
      counter = 0;
      pub.publish(cloud_target);
    }
  }
}//end callback

/*int main (int argc, char** argv)
{
  ROS_INFO("Starting Filter Node");
  counter = 0;
  saveCounter = 0;
  ros::init(argc, argv, "pcl_filter");
  ros::NodeHandle nh;

  ros::spin();
  return (0);
