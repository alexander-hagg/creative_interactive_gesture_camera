#ifndef KEYFRAME_LOOP_DETECTOR
#define KEYFRAME_LOOP_DETECTOR

//#include <mrpt/utils/CTimeLogger.h> //Profiling
#include "FrameRGBD.h"
#include "Miscellaneous.h"
#include <vector>
#include <Eigen/LU>
#include "VisualFeatureMatcher_Generic.h"
#include "Visual3DRigidTransformationEstimator.h"
#include "ICPPoseRefiner.h"

/*!This class implements a very straightforward approach for loop detection. It is based on the resulting number of inliers from the feature matching process. This implementation matches the last added keyframe against all the previous keyframes to find the ones that best match. If the number of inliers are greater than a certain threshold, then it considers that both the matched previous keyframe and the current keyframe belongs to the same place (loop detection). This class returns not only the loops between keyframes, but also the spatial constraints between keyframes.*/
class KeyframeLoopDetector
{

private:
  /*!Vector of all the added keyframes.*/
  std::vector<FrameRGBD*> keyframes;
  /*!Vector of the 6D poses corresponding to each keyframe.*/
  std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f > > poses;
  /*!Vector of the 2D keypoints corresponding to the intensity image of each keyframe.*/
  std::vector<std::vector<cv::KeyPoint> > keypointsList;
  /*!Vector of the decriptors corresponding to the 2D keypoints of each keyframe.*/
  std::vector<cv::Mat> descriptorsList;
  /*!Threshold that specifies the minimum number of inliers needed to detect a loop.*/
  int numberInliersThreshold;
  Eigen::Matrix4f relativePose;
  //mrpt::utils::CTimeLogger profiler;

public:
  KeyframeLoopDetector(const int=30);
  /*!Adds a new keyframe to the loop detector.*/
  void addKeyframe(FrameRGBD* keyframe);
  /*!Adds a 6D pose that specifies the position and orientation of the camera from which the keyframe has been grabbed.*/
  void addPose(Eigen::Matrix4f& pose);
  /*!Adds the keypoints corresponding the intensity image of the added keyframe.*/
  void addKeypoints(std::vector<cv::KeyPoint>& keypoints);
  /*!Adds the descriptors corresponding to the keypoints of the added keyframe.*/
  void addDescriptors(cv::Mat& descriptors);
  /*!Performs the loop detection process matching the added keyframe against previous keyframes. It returns a vector of 6D relatives poses that defines spatial constraints between keyframes, a vector of 6x6 information matrices that represents the goodness of the matches. It also returns a vector of keyframe indexes and a keyframe index that determines the "origins" and "end" of each loop.*/
  void detectLoop(VisualFeatureMatcher_Generic& matcher,
                  Visual3DRigidTransformationEstimator& rigidTransformEstimator,
                  ICPPoseRefiner& poseRefiner,
                  std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> >& relativePoses,
                  std::vector<Eigen::Matrix<double,6,6>, Eigen::aligned_allocator<Eigen::Matrix<double,6,6 > > >& infMatrices,
                  std::vector<int>& fromIndexes,int& toIndex);
  /*!Returns a vector of all the keyframe poses.*/
  void getPoses(std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f > >&);
  /*!Returns a vector of all the keyframes*/
  void getKeyframes(std::vector<FrameRGBD*>&);
  /*!Adds a new pose accumulating the provided relative pose.*/
  void accumulateRelativePose(Eigen::Matrix4f& relPose);
  /*!Returns the current keyframe pose.*/
  void getCurrentPose(Eigen::Matrix4f& pose);
  /*!Sets the vector of keyframe poses with the provided vector of poses.*/
  void setPoses(std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f > >&);
  /*!Save profiling results for loop detection*/
  void saveLoopProfiling(std::string fileName);
  /*!Get pointer to the list of keyframes*/
  inline std::vector<FrameRGBD*>* keyframesPointer(){return &keyframes;}
  /*!Get pointer to the list of keypoints*/
  inline std::vector<std::vector<cv::KeyPoint> >* keypointsListPointer(){return &keypointsList;}
  /*!Get pointer to the list of descriptors*/
  inline std::vector<cv::Mat>* descriptorsListPointer(){return &descriptorsList;}
};

#endif
