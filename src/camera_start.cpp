////////////////////////////////////////////////////////////////////////////////
// SoftKinetic DepthSense SDK
//
// COPYRIGHT AND CONFIDENTIALITY NOTICE - SOFTKINETIC CONFIDENTIAL
// INFORMATION
//
// All rights reserved to SOFTKINETIC SENSORS NV (a
// company incorporated and existing under the laws of Belgium, with
// its principal place of business at Boulevard de la Plainelaan 15,
// 1050 Brussels (Belgium), registered with the Crossroads bank for
// enterprises under company number 0811 341 454 - "Softkinetic
// Sensors").
//
// The source code of the SoftKinetic DepthSense Camera Drivers is
// proprietary and confidential information of Softkinetic Sensors NV.
//
// For any question about terms and conditions, please contact:
// info@softkinetic.com Copyright (c) 2002-2012 Softkinetic Sensors NV
////////////////////////////////////////////////////////////////////////////////


#ifdef _MSC_VER
#include <windows.h>
#endif

#include <stdio.h>
#include <vector>
#include <exception>
#include <iostream>
#include <fstream>

//ros include files
#include <ros/ros.h>
#include <std_msgs/Int32.h>
#include <sensor_msgs/PointCloud2.h>
#include <image_transport/image_transport.h>
#include <pcl_ros/point_cloud.h>
#include <pcl_ros/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/range_image/range_image.h>
#include <pcl/visualization/cloud_viewer.h>

#include <DepthSense.hxx>

#include "cameraSync.h"

using namespace DepthSense;
using namespace std;

typedef pcl::PointCloud<pcl::PointXYZ> PointCloud;

Context g_context;
DepthNode g_dnode;
ColorNode g_cnode;
AudioNode g_anode;

uint32_t g_aFrames = 0;
uint32_t g_cFrames = 0;
uint32_t g_dFrames = 0;

bool g_bDeviceFound = false;

ProjectionHelper* g_pProjHelper = NULL;
StereoCameraParameters g_scp;

ros::Publisher pub_cloud;
image_transport::Publisher pub_rgb;

ros::Publisher pub_test;

sensor_msgs::Image image;
std_msgs::Int32 test_int;
pcl::PointCloud<pcl::PointXYZ> cloud;

cameraSync* syncObject;

/*----------------------------------------------------------------------------*/
// New audio sample event handler
void onNewAudioSample(AudioNode node, AudioNode::NewSampleReceivedData data)
{
    //printf("A#%u: %d\n",g_aFrames,data.audioData.size());
    g_aFrames++;
}

/*----------------------------------------------------------------------------*/
// New color sample event handler
void onNewColorSample(ColorNode node, ColorNode::NewSampleReceivedData data)
{   
    image.header.frame_id = "/base_link";
    image.header.stamp = ros::Time::now();
    int count = -3;
    int32_t w, h;
    FrameFormat_toResolution(data.captureConfiguration.frameFormat,&w,&h);
    image.width = w;
    image.height = h;
    image.encoding = "bgr8";
    image.data.resize(w*h*3);
    for(int i = 0;i < h;i++){
	    for(int j = 0;j < w; j++){
	      count+=3;
        image.data[count] = data.colorMap[count];
        image.data[count+1] = data.colorMap[count+1];
        image.data[count+2] = data.colorMap[count+2];    
	    }
    }
       
    //test_int.data = count;
    //pub_test.publish(test_int);
    pub_rgb.publish(image);
    
    //syncObject->imageCallback(image&);
    
    g_cFrames++;
}

/*----------------------------------------------------------------------------*/
// New depth sample event varsace tieshandler
void onNewDepthSample(DepthNode node, DepthNode::NewSampleReceivedData data)
{
    //printf("Z#%u: %d %d %d\n",g_dFrames,data.vertices[500].x,data.vertices[500].y,data.vertices[500].z);
    int count = -1;
    cloud.header.frame_id = "/base_link";
    cloud.header.stamp = ros::Time::now().toSec();
    // Project some 3D points in the Color Frame
    if (!g_pProjHelper)
    {
        g_pProjHelper = new ProjectionHelper (data.stereoCameraParameters);
        g_scp = data.stereoCameraParameters;
    }
    else if (g_scp != data.stereoCameraParameters)
    {
        g_pProjHelper->setStereoCameraParameters(data.stereoCameraParameters);
        g_scp = data.stereoCameraParameters;
    }

    int32_t w, h;
    FrameFormat_toResolution(data.captureConfiguration.frameFormat,&w,&h);
    int cx = w/2;
    int cy = h/2;  

    /*Vertex p3DPoints[4];

    p3DPoints[0] = data.vertices[(cy-h/4)*w+cx-w/4];
    p3DPoints[1] = data.vertices[(cy-h/4)*w+cx+w/4];
    p3DPoints[2] = data.vertices[(cy+h/4)*w+cx+w/4];
    p3DPoints[3] = data.vertices[(cy+h/4)*w+cx-w/4];
    
    Point2D p2DPoints[4];
    g_pProjHelper->get2DCoordinates ( p3DPoints, p2DPoints, 4, CAMERA_PLANE_COLOR);*/
    
    g_dFrames++;
   
    cloud.height = h;
    cloud.width = w;
    cloud.is_dense = true;
    cloud.points.resize(w*h); 

    for(int i = 0;i < h;i++){
	    for(int j = 0;j < w; j++){
	       count++;
         cloud.points[count].x = data.verticesFloatingPoint[count].x;
	       cloud.points[count].y = data.verticesFloatingPoint[count].y;
         if(data.verticesFloatingPoint[count].z == 32001){
		      cloud.points[count].z = 0;
      	 }else{
	 	      cloud.points[count].z = data.verticesFloatingPoint[count].z;
         }
	    }
    }

    pub_cloud.publish (cloud);
    g_context.quit();
}

/*----------------------------------------------------------------------------*/
void configureAudioNode()
{
    g_anode.newSampleReceivedEvent().connect(&onNewAudioSample);

    AudioNode::Configuration config = g_anode.getConfiguration();
    config.sampleRate = 44100;

    try 
    {
        g_context.requestControl(g_anode,0);

        g_anode.setConfiguration(config);
        
        g_anode.setInputMixerLevel(0.5f);
    }
    catch (ArgumentException& e)
    {
        printf("Argument Exception: %s\n",e.what());
    }
    catch (UnauthorizedAccessException& e)
    {
        printf("Unauthorized Access Exception: %s\n",e.what());
    }
    catch (ConfigurationException& e)
    {
        printf("Configuration Exception: %s\n",e.what());
    }
    catch (StreamingException& e)
    {
        printf("Streaming Exception: %s\n",e.what());
    }
    catch (TimeoutException&)
    {
        printf("TimeoutException\n");
    }
}

/*----------------------------------------------------------------------------*/
void configureDepthNode()
{
    g_dnode.newSampleReceivedEvent().connect(&onNewDepthSample);

    DepthNode::Configuration config = g_dnode.getConfiguration();
    config.frameFormat = FRAME_FORMAT_QVGA;
    config.framerate = 25;
    config.mode = DepthNode::CAMERA_MODE_CLOSE_MODE;
    config.saturation = true;

    g_dnode.setEnableVerticesFloatingPoint(true);
    g_dnode.setEnableDepthMapFloatingPoint(true);

    try 
    {
        g_context.requestControl(g_dnode,0);

        g_dnode.setConfiguration(config);
    }
    catch (ArgumentException& e)
    {
        printf("Argument Exception: %s\n",e.what());
    }
    catch (UnauthorizedAccessException& e)
    {
        printf("Unauthorized Access Exception: %s\n",e.what());
    }
    catch (IOException& e)
    {
        printf("IO Exception: %s\n",e.what());
    }
    catch (InvalidOperationException& e)
    {
        printf("Invalid Operation Exception: %s\n",e.what());
    }
    catch (ConfigurationException& e)
    {
        printf("Configuration Exception: %s\n",e.what());
    }
    catch (StreamingException& e)
    {
        printf("Streaming Exception: %s\n",e.what());
    }
    catch (TimeoutException&)
    {
        printf("TimeoutException\n");
    }

}

/*----------------------------------------------------------------------------*/
void configureColorNode()
{
    // connect new color sample handler
    g_cnode.newSampleReceivedEvent().connect(&onNewColorSample);

    ColorNode::Configuration config = g_cnode.getConfiguration();
    config.frameFormat = FRAME_FORMAT_VGA;
    config.compression = COMPRESSION_TYPE_MJPEG;
    config.powerLineFrequency = POWER_LINE_FREQUENCY_50HZ;
    config.framerate = 25;

    g_cnode.setEnableColorMap(true);

    try 
    {
        g_context.requestControl(g_cnode,0);

        g_cnode.setConfiguration(config);
    }
    catch (ArgumentException& e)
    {
        printf("Argument Exception: %s\n",e.what());
    }
    catch (UnauthorizedAccessException& e)
    {
        printf("Unauthorized Access Exception: %s\n",e.what());
    }
    catch (IOException& e)
    {
        printf("IO Exception: %s\n",e.what());
    }
    catch (InvalidOperationException& e)
    {
        printf("Invalid Operation Exception: %s\n",e.what());
    }
    catch (ConfigurationException& e)
    {
        printf("Configuration Exception: %s\n",e.what());
    }
    catch (StreamingException& e)
    {
        printf("Streaming Exception: %s\n",e.what());
    }
    catch (TimeoutException&)
    {
        printf("TimeoutException\n");
    }
}

/*----------------------------------------------------------------------------*/
void configureNode(Node node)
{
    if ((node.is<DepthNode>())&&(!g_dnode.isSet()))
    {
        g_dnode = node.as<DepthNode>();
        configureDepthNode();
        g_context.registerNode(node);
    }

    if ((node.is<ColorNode>())&&(!g_cnode.isSet()))
    {
        g_cnode = node.as<ColorNode>();
        configureColorNode();
        g_context.registerNode(node);
    }

    if ((node.is<AudioNode>())&&(!g_anode.isSet()))
    {
        g_anode = node.as<AudioNode>();
        configureAudioNode();
        g_context.registerNode(node);
    }
}

/*----------------------------------------------------------------------------*/
void onNodeConnected(Device device, Device::NodeAddedData data)
{
    configureNode(data.node);
}

/*----------------------------------------------------------------------------*/
void onNodeDisconnected(Device device, Device::NodeRemovedData data)
{
    if (data.node.is<AudioNode>() && (data.node.as<AudioNode>() == g_anode))
        g_anode.unset();
    if (data.node.is<ColorNode>() && (data.node.as<ColorNode>() == g_cnode))
        g_cnode.unset();
    if (data.node.is<DepthNode>() && (data.node.as<DepthNode>() == g_dnode))
        g_dnode.unset();
    printf("Node disconnected\n");
}

/*----------------------------------------------------------------------------*/
void onDeviceConnected(Context context, Context::DeviceAddedData data)
{
    if (!g_bDeviceFound)
    {
        data.device.nodeAddedEvent().connect(&onNodeConnected);
        data.device.nodeRemovedEvent().connect(&onNodeDisconnected);
        g_bDeviceFound = true;
    }
}

/*----------------------------------------------------------------------------*/
void onDeviceDisconnected(Context context, Context::DeviceRemovedData data)
{
    g_bDeviceFound = false;
    printf("Device disconnected\n");
}

/*----------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
    ros::init (argc, argv, "pub_pcl");
    ros::NodeHandle nh;
    image_transport::ImageTransport it(nh);


    pub_cloud = nh.advertise<PointCloud> ("points2", 1);
    pub_rgb = it.advertise ("rgb_data", 1);
    pub_test = nh.advertise<std_msgs::Int32> ("test",1);
    
    syncObject = new cameraSync();
    
    g_context = Context::create("localhost");

    g_context.deviceAddedEvent().connect(&onDeviceConnected);
    g_context.deviceRemovedEvent().connect(&onDeviceDisconnected);

    // Get the list of currently connected devices
    vector<Device> da = g_context.getDevices();

    // We are only interested in the first device
    if (da.size() >= 1)
    {
        g_bDeviceFound = true;

        da[0].nodeAddedEvent().connect(&onNodeConnected);
        da[0].nodeRemovedEvent().connect(&onNodeDisconnected);

        vector<Node> na = da[0].getNodes();
        
        printf("Found %u nodes\n",na.size());
        
        for (int n = 0; n < (int)na.size();n++)
            configureNode(na[n]);
    }

    while(ros::ok()){
    	g_context.startNodes();
    	g_context.run();
    }
    if (g_cnode.isSet()) g_context.unregisterNode(g_cnode);
    if (g_dnode.isSet()) g_context.unregisterNode(g_dnode);
    if (g_anode.isSet()) g_context.unregisterNode(g_anode);

    if (g_pProjHelper)
        delete g_pProjHelper;
    g_context.stopNodes();
    return 0;
}
