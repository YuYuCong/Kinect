#include <windows.h>
#include <iostream> 
#include <NuiApi.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

bool tracked[NUI_SKELETON_COUNT] = { FALSE };
CvPoint skeletonPoint[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT] = { cvPoint(0,0) };
CvPoint colorPoint[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT] = { cvPoint(0,0) };

///2017.11.06添加
vector<CvPoint> hand_center;

void getColorImage(HANDLE &colorEvent, HANDLE &colorStreamHandle, Mat &colorImage);
void getDepthImage(HANDLE &depthEvent, HANDLE &depthStreamHandle, Mat &depthImage);
void getSkeletonImage(HANDLE &skeletonEvent, Mat &skeletonImage, Mat &colorImage, Mat &depthImage);
void drawSkeleton(Mat &image, CvPoint pointSet[], int witchone);
void getTheContour(Mat &image, int whichone, Mat &mask);

///2017.11.06添加
void drawHand(Mat&mask, Mat&hand, vector<CvPoint> &hand_center, int i);
///2017.11.19添加
void get_hand_center(Mat &image, CvPoint pointSet[], int whichone);
//void detectHand(Mat&hand);
int otsuThreshold(IplImage *frame);


int main(int argc, char *argv[])
{
	Mat colorImage;
	colorImage.create(480, 640, CV_8UC3);
	Mat depthImage;
	depthImage.create(240, 320, CV_8UC3);
	Mat skeletonImage;
	skeletonImage.create(240, 320, CV_8UC3);
	Mat mask;
	mask.create(240, 320, CV_8UC3);
	///2017.11.06添加
	Mat hand;    
	Mat grayImage, thresholdImage;//临时变量和目标图的定义  
	int threshold_number;
	vector<vector<Point>> contours;
	int k = 1;



	HANDLE colorEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE depthEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE skeletonEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	HANDLE colorStreamHandle = NULL;
	HANDLE depthStreamHandle = NULL;

	HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR |
		NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON);
	if (hr != S_OK)
	{
		cout << "NuiInitialize failed" << endl;
		return hr;
	}
	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, NULL, 4, colorEvent, &colorStreamHandle);
	if (hr != S_OK)
	{
		cout << "Open the color Stream failed" << endl;
		NuiShutdown();
		return hr;
	}
	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_320x240, NULL, 2, depthEvent, &depthStreamHandle);
	if (hr != S_OK)
	{
		cout << "Open the depth Stream failed" << endl;
		NuiShutdown();
		return hr;
	}
	hr = NuiSkeletonTrackingEnable(skeletonEvent, 0);//打开骨骼跟踪事件   
	if (hr != S_OK)
	{
		cout << "NuiSkeletonTrackingEnable failed" << endl;
		NuiShutdown();
		return hr;
	}

	///2017.11.06添加
	namedWindow("hand", CV_WINDOW_AUTOSIZE);

	namedWindow("mask", CV_WINDOW_AUTOSIZE);
	namedWindow("colorImage", CV_WINDOW_AUTOSIZE);
	namedWindow("depthImage", CV_WINDOW_AUTOSIZE);
	namedWindow("skeletonImage", CV_WINDOW_AUTOSIZE);
	namedWindow("skeletonImage", CV_WINDOW_AUTOSIZE);
	
	while (1)
	{
		if (WaitForSingleObject(colorEvent, 0) == 0)
			getColorImage(colorEvent, colorStreamHandle, colorImage);
		if (WaitForSingleObject(depthEvent, 0) == 0)
			getDepthImage(depthEvent, depthStreamHandle, depthImage);
		//这里使用INFINITE是为了避免没有激活skeletonEvent而跳过此代码出现colorimage频闪的现象 
		if (WaitForSingleObject(skeletonEvent, INFINITE) == 0)
			getSkeletonImage(skeletonEvent, skeletonImage, colorImage, depthImage);

		for (int i = 0; i < 6; i++)
		{
			if (tracked[i] == TRUE)
			{
				k ++;
				///2017.11.19添加
				hand.setTo(0);

				mask.setTo(0);
				getTheContour(depthImage, i, mask);

				//-----------------------------------【图像的预处理】---------------------------------------    
				//      描述：图像的读入，灰度化，二值化及滤波     
				//-----------------------------------------------------------------------------------------------      
				//【1】载入原始图和Mat变量定义  
				if (mask.rows == 0 || hand_center.size() == 0)
				{
				}
				else if (   35 < hand_center[0].x && hand_center[0].x < (240 - 35)
					&& 35 < hand_center[0].y && hand_center[0].y < (320 - 35)   ) 
				{

					///2017.11.06添加
					drawHand(mask, hand, hand_center, i);

					Mat srcImage = hand;  //工程目录下应该有一张名为齿轮.jpg的素材图    
					Mat grayImage, thresholdImage;//临时变量和目标图的定义  

					//【2】显示原始图    
					imshow("【1.原始图】", srcImage);

					////【3】提取感兴趣域  
					////-----------------------------------以下是我添加的---------------------------------------  
					////使用cvsetImageROI函数
					////cvsetImageROI(src，cvRect(x, y, width, height))
					////其中x和y为ROI区域的起点，width和height为宽和高，
					////对src提取感兴趣区域后，再对src进行图像处理时只针对提取的感兴趣进行处理，在图像处理时要注意这一点。
					////使用该函数时要对通过cvResetImageROI()函数释放ROI，否则对图像进行处理时只对ROI区域进行对比。
					////-----------------------------------以上是我添加的---------------------------------------    
					////Rect ROIim(300, 100, 600, 600);    //参数需视图片修改
					////Mat ROIsrc;
					////srcImage(ROIim).copyTo(ROIsrc);
					////namedWindow("【2.roiimage】", WINDOW_NORMAL);
					////imshow("【2.roiimage】", ROIsrc);

					//【4】转为灰度图，进行图像平滑    
					cvtColor(srcImage, grayImage, CV_BGR2GRAY);//转化边缘检测后的图为灰度图     
					imshow("【3.灰度图】", grayImage);

					//【5】图像的二值化，采用大津法 
					//-----------------------------------以下是我改动的---------------------------------------    
					IplImage * frame_gray;
					frame_gray = &IplImage(grayImage);
					int threshold_number = otsuThreshold(frame_gray);           //调用大津法函数
					threshold(grayImage, thresholdImage, threshold_number, 255, CV_THRESH_BINARY);
					imshow("【4.二值图】", thresholdImage);
					//-----------------------------------以上是我改动的---------------------------------------   

					////【6】中值滤波  
					//medianBlur(thresholdImage, thresholdImage, 3);
					//namedWindow("【5.滤波处理结果图】", WINDOW_NORMAL);
					//imshow("【5.滤波处理结果图】", thresholdImage);

					//-----------------------------------【图像分割】---------------------------------------    
					//      描述：边缘检测，轮廓提取
					//-----------------------------------------------------------------------------------------------  
					if(k==30)
					{
						//waitKey(0);
						cout << k<<endl;
						//【1】获取轮廓
						findContours(thresholdImage,            //图像    
							contours,               //轮廓点     
							CV_RETR_EXTERNAL,       //获取轮廓的方法（这里获取外围轮廓）    
							CV_CHAIN_APPROX_NONE    //轮廓近似的方法（这里不近似，获取全部轮廓）  
						);
						//测试轮廓条数  
						if (contours.size() == 0) {}
						else {
							cout << "轮廓：" << contours.size() << "条" << endl;
							for (int i = 0; i < contours.size(); i++)
							{
								cout << "轮廓长：" << contours[i].size() << endl;
							}
							long cmin = 50;           //参数需视图片修改
							long cmax = 500;
							vector<vector<Point>>::const_iterator itc = contours.begin();
							while (itc != contours.end())
							{
								if (itc->size() < cmin || itc->size() > cmax)
								{
									itc = contours.erase(itc);
									//itc = contours.begin();
								}
								else
									++itc;
							}

							cout << "有效轮廓：" << contours.size() << "条" << endl;
							if (contours.size() == 0) {}
							else
							{
								for (int i = 0; i < contours.size(); i++)
								{
									cout << "有效轮廓长：" << contours[i].size() << endl;
								}
							}
							
						}
						k = 1;
					}
					////srcImage.release();
					////grayImage.release();
					////thresholdImage.release();
					////ContoursIm.release();
					

				}
				else {}
				hand_center.clear();
				
				tracked[i] = FALSE;
				break;
			}
		}

		imshow("mask", mask);
		imshow("colorImage", colorImage);
		imshow("depthImage", depthImage);
		imshow("skeletonImage", skeletonImage);

		if (cvWaitKey(1) == 27)
		{
			break;
		}
	}



	NuiShutdown();
	return 0;
}


void getColorImage(HANDLE &colorEvent, HANDLE &colorStreamHandle, Mat &colorImage)
{
	const NUI_IMAGE_FRAME *colorFrame = NULL;

	NuiImageStreamGetNextFrame(colorStreamHandle, 0, &colorFrame);
	INuiFrameTexture *pTexture = colorFrame->pFrameTexture;

	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	if (LockedRect.Pitch != 0)
	{
		for (int i = 0; i<colorImage.rows; i++)
		{
			uchar *ptr = colorImage.ptr<uchar>(i);  //第i行的指针					
													//每个字节代表一个颜色信息，直接使用uchar
			uchar *pBuffer = (uchar*)(LockedRect.pBits) + i * LockedRect.Pitch;
			for (int j = 0; j<colorImage.cols; j++)
			{
				ptr[3 * j] = pBuffer[4 * j];  //内部数据是4个字节，0-1-2是BGR，第4个现在未使用 
				ptr[3 * j + 1] = pBuffer[4 * j + 1];
				ptr[3 * j + 2] = pBuffer[4 * j + 2];
			}
		}
	}
	else
	{
		cout << "捕捉色彩图像出现错误" << endl;
	}

	pTexture->UnlockRect(0);
	NuiImageStreamReleaseFrame(colorStreamHandle, colorFrame);
}

void getDepthImage(HANDLE &depthEvent, HANDLE &depthStreamHandle, Mat &depthImage)
{
	const NUI_IMAGE_FRAME *depthFrame = NULL;

	NuiImageStreamGetNextFrame(depthStreamHandle, 0, &depthFrame);
	INuiFrameTexture *pTexture = depthFrame->pFrameTexture;

	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	RGBQUAD q;

	if (LockedRect.Pitch != 0)
	{
		for (int i = 0; i<depthImage.rows; i++)
		{
			uchar *ptr = depthImage.ptr<uchar>(i);
			uchar *pBuffer = (uchar*)(LockedRect.pBits) + i * LockedRect.Pitch;
			USHORT *pBufferRun = (USHORT*)pBuffer;

			for (int j = 0; j<depthImage.cols; j++)
			{
				int player = pBufferRun[j] & 7;
				int data = (pBufferRun[j] & 0xfff8) >> 3;

				uchar imageData = 255 - (uchar)(256 * data / 0x0fff);
				q.rgbBlue = q.rgbGreen = q.rgbRed = 0;

				switch (player)
				{
				case 0:
					q.rgbRed = imageData / 2;
					q.rgbBlue = imageData / 2;
					q.rgbGreen = imageData / 2;
					break;
				case 1:
					q.rgbRed = imageData;
					break;
				case 2:
					q.rgbGreen = imageData;
					break;
				case 3:
					q.rgbRed = imageData / 4;
					q.rgbGreen = q.rgbRed * 4;  //这里利用乘的方法，而不用原来的方法可以避免不整除的情况 
					q.rgbBlue = q.rgbRed * 4;  //可以在后面的getTheContour()中配合使用，避免遗漏一些情况 
					break;
				case 4:
					q.rgbBlue = imageData / 4;
					q.rgbRed = q.rgbBlue * 4;
					q.rgbGreen = q.rgbBlue * 4;
					break;
				case 5:
					q.rgbGreen = imageData / 4;
					q.rgbRed = q.rgbGreen * 4;
					q.rgbBlue = q.rgbGreen * 4;
					break;
				case 6:
					q.rgbRed = imageData / 2;
					q.rgbGreen = imageData / 2;
					q.rgbBlue = q.rgbGreen * 2;
					break;
				case 7:
					q.rgbRed = 255 - (imageData / 2);
					q.rgbGreen = 255 - (imageData / 2);
					q.rgbBlue = 255 - (imageData / 2);
				}
				ptr[3 * j] = q.rgbBlue;
				ptr[3 * j + 1] = q.rgbGreen;
				ptr[3 * j + 2] = q.rgbRed;
			}
		}
	}
	else
	{
		cout << "捕捉深度图像出现错误" << endl;
	}

	pTexture->UnlockRect(0);
	NuiImageStreamReleaseFrame(depthStreamHandle, depthFrame);
}

void getSkeletonImage(HANDLE &skeletonEvent, Mat &skeletonImage, Mat &colorImage, Mat &depthImage)
{
	NUI_SKELETON_FRAME skeletonFrame = { 0 };
	bool bFoundSkeleton = false;

	if (NuiSkeletonGetNextFrame(0, &skeletonFrame) == S_OK)
	{
		for (int i = 0; i < NUI_SKELETON_COUNT; i++)
		{
			if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED)
			{
				bFoundSkeleton = true;
				break;
			}
		}
	}
	else
	{
		cout << "没有找到合适的骨骼帧" << endl;
		return;
	}

	if (!bFoundSkeleton)
	{
		return;
	}

	NuiTransformSmooth(&skeletonFrame, NULL);//平滑骨骼帧,消除抖动   
	skeletonImage.setTo(0);

	for (int i = 0; i < NUI_SKELETON_COUNT; i++)
	{
		if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&
			skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)
		{
			float fx, fy;

			for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++)//所有的坐标转化为深度图的坐标   
			{
				NuiTransformSkeletonToDepthImage(skeletonFrame.SkeletonData[i].SkeletonPositions[j], &fx, &fy);
				skeletonPoint[i][j].x = (int)fx;
				skeletonPoint[i][j].y = (int)fy;
			}

			for (int j = 0; j<NUI_SKELETON_POSITION_COUNT; j++)
			{
				if (skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED)//跟踪点一用有三种状态：1没有被跟踪到，2跟踪到，3根据跟踪到的估计到   
				{
					LONG colorx, colory;
					NuiImageGetColorPixelCoordinatesFromDepthPixel ( NUI_IMAGE_RESOLUTION_640x480, 0,
						skeletonPoint[i][j].x, skeletonPoint[i][j].y, 0, &colorx, &colory);
					colorPoint[i][j].x = int(colorx);
					colorPoint[i][j].y = int(colory); //存储坐标点 
					circle(colorImage, colorPoint[i][j], 4, cvScalar(0, 255, 255), 1, 8, 0);
					circle(skeletonImage, skeletonPoint[i][j], 3, cvScalar(0, 255, 255), 1, 8, 0);

					tracked[i] = TRUE;
				}
			}

			drawSkeleton(skeletonImage, skeletonPoint[i], i);
			drawSkeleton(colorImage, colorPoint[i], i);

			///2017.11.19添加
			get_hand_center(skeletonImage, skeletonPoint[i], i);
		}
	}
}

void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone)
{
	CvScalar color;
	switch (whichone) //跟踪不同的人显示不同的颜色 
	{
	case 0:
		color = cvScalar(255);
		break;
	case 1:
		color = cvScalar(0, 255);
		break;
	case 2:
		color = cvScalar(0, 0, 255);
		break;
	case 3:
		color = cvScalar(255, 255, 0);
		break;
	case 4:
		color = cvScalar(255, 0, 255);
		break;
	case 5:
		color = cvScalar(0, 255, 255);
		break;
	}

	if ((pointSet[NUI_SKELETON_POSITION_HEAD].x != 0 || pointSet[NUI_SKELETON_POSITION_HEAD].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HEAD], pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SPINE].x != 0 || pointSet[NUI_SKELETON_POSITION_SPINE].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], pointSet[NUI_SKELETON_POSITION_SPINE], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SPINE].x != 0 || pointSet[NUI_SKELETON_POSITION_SPINE].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HIP_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_CENTER].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SPINE], pointSet[NUI_SKELETON_POSITION_HIP_CENTER], color, 2);

	//左上肢 
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT], pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT], pointSet[NUI_SKELETON_POSITION_WRIST_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HAND_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_HAND_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_WRIST_LEFT], pointSet[NUI_SKELETON_POSITION_HAND_LEFT], color, 2);

	//右上肢 
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT], pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT], pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HAND_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_HAND_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT], pointSet[NUI_SKELETON_POSITION_HAND_RIGHT], color, 2);

	//左下肢 
	if ((pointSet[NUI_SKELETON_POSITION_HIP_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HIP_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_CENTER], pointSet[NUI_SKELETON_POSITION_HIP_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_HIP_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_LEFT], pointSet[NUI_SKELETON_POSITION_KNEE_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_KNEE_LEFT], pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_FOOT_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_FOOT_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT], pointSet[NUI_SKELETON_POSITION_FOOT_LEFT], color, 2);

	//右下肢 
	if ((pointSet[NUI_SKELETON_POSITION_HIP_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_CENTER], pointSet[NUI_SKELETON_POSITION_HIP_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_RIGHT], pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT], pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_FOOT_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_FOOT_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT], pointSet[NUI_SKELETON_POSITION_FOOT_RIGHT], color, 2);
}

//根据给定的深度数据的关系（在getDepthImage()中的）确定不同的跟踪目标 
void getTheContour(Mat &image, int whichone, Mat &mask)
{
	for (int i = 0; i<image.rows; i++)
	{
		uchar *ptr = image.ptr<uchar>(i);
		uchar *ptrmask = mask.ptr<uchar>(i);
		for (int j = 0; j<image.cols; j++)
		{
			if (ptr[3 * j] == 0 && ptr[3 * j + 1] == 0 && ptr[3 * j + 2] == 0) //都为0的时候予以忽略 
			{
				ptrmask[3 * j] = ptrmask[3 * j + 1] = ptrmask[3 * j + 2] = 0;
			}
			else if (ptr[3 * j] == 0 && ptr[3 * j + 1] == 0 && ptr[3 * j + 2] != 0)//ID为1的时候，显示绿色 
			{
				ptrmask[3 * j] = 0;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 0;
			}
			else if (ptr[3 * j] == 0 && ptr[3 * j + 1] != 0 && ptr[3 * j + 2] == 0)//ID为2的时候，显示红色 
			{
				ptrmask[3 * j] = 0;
				ptrmask[3 * j + 1] = 0;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == ptr[3 * j + 1] && ptr[3 * j] == 4 * ptr[3 * j + 2])//ID为3的时候 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 0;
			}
			else if (4 * ptr[3 * j] == ptr[3 * j + 1] && ptr[3 * j + 1] == ptr[3 * j + 2])//ID为4的时候 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 0;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == 4 * ptr[3 * j + 1] && ptr[3 * j] == ptr[3 * j + 2])//ID为5的时候 
			{
				ptrmask[3 * j] = 0;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == 2 * ptr[3 * j + 1] && ptr[3 * j + 1] == ptr[3 * j + 2])//ID为6的时候 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == ptr[3 * j + 1] && ptr[3 * j] == ptr[3 * j + 2])//ID为7的时候或者ID为0的时候，显示蓝色 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 0;
				ptrmask[3 * j + 2] = 0;
			}
			else
			{
				cout << "如果输出这段代码，说明有遗漏的情况，请查询getTheContour函数" << endl;
			}
		}
	}
}


void get_hand_center(Mat &image, CvPoint pointSet[], int whichone)
{
	hand_center.push_back(pointSet[NUI_SKELETON_POSITION_HAND_RIGHT]);
}


///2017.11.06添加
void drawHand(Mat&mask, Mat&hand, vector<CvPoint> &hand_center, int i)
{
	Rect rect;

	if (hand_center.size() == 0) 
	{
	}
	else{
		CvScalar color;
		switch (i) //跟踪不同的人显示不同的颜色 
		{
		case 0:
			color = cvScalar(255);
			break;
		case 1:
			color = cvScalar(0, 255);
			break;
		case 2:
			color = cvScalar(0, 0, 255);
			break;
		case 3:
			color = cvScalar(255, 255, 0);
			break;
		case 4:
			color = cvScalar(255, 0, 255);
			break;
		case 5:
			color = cvScalar(0, 255, 255);
			break;
		}
		for (int i = 0; i < hand_center.size(); i++)
		{
			///2017.11.19添加
			if (hand_center.size() == 0)
			{
			}
			else {
				//circle(mask, hand_center[0], 5, Scalar(0, 0, 0), -1, 8, 0);
				
				//防止超出`
				if (35 < hand_center[0].x && hand_center[0].x< (240 - 35)
					&& 35 < hand_center[0].y && hand_center[0].y < (320 - 35))
				{
					rect.x = hand_center[0].x - 30;
					rect.y = hand_center[0].y - 30;
					rect.width = 60;
					rect.height = 60;
					mask(rect).copyTo(hand);
					imshow("hand", hand);
				}
				else {}
			}
		}
	}
}

/*void detectHand(Mat&hand)
{
	//-----------------------------------【图像的预处理】---------------------------------------    
	//      描述：图像的读入，灰度化，二值化及滤波     
	//-----------------------------------------------------------------------------------------------      
	//【1】载入原始图和Mat变量定义  
	if (hand.rows == 0)
	{
	}
	else {
	Mat srcImage = hand;  //工程目录下应该有一张名为齿轮.jpg的素材图    
	Mat grayImage, thresholdImage;//临时变量和目标图的定义  

	//【2】显示原始图    
	namedWindow("【1.原始图】", WINDOW_NORMAL);
	imshow("【1.原始图】", srcImage);

	//【3】提取感兴趣域  
	//-----------------------------------以下是我添加的---------------------------------------  
	//使用cvsetImageROI函数
	//cvsetImageROI(src，cvRect(x, y, width, height))
	//其中x和y为ROI区域的起点，width和height为宽和高，
	//对src提取感兴趣区域后，再对src进行图像处理时只针对提取的感兴趣进行处理，在图像处理时要注意这一点。
	//使用该函数时要对通过cvResetImageROI()函数释放ROI，否则对图像进行处理时只对ROI区域进行对比。
	//-----------------------------------以上是我添加的---------------------------------------    
	//Rect ROIim(300, 100, 600, 600);    //参数需视图片修改
	//Mat ROIsrc;
	//srcImage(ROIim).copyTo(ROIsrc);
	//namedWindow("【2.roiimage】", WINDOW_NORMAL);
	//imshow("【2.roiimage】", ROIsrc);

	//【4】转为灰度图，进行图像平滑    
	cvtColor(srcImage, grayImage, CV_BGR2GRAY);//转化边缘检测后的图为灰度图     
	namedWindow("【3.灰度图】", WINDOW_NORMAL);
	imshow("【3.灰度图】", grayImage);

	//【5】图像的二值化，采用大津法 
	//-----------------------------------以下是我改动的---------------------------------------    
	IplImage * frame_gray;
	frame_gray = &IplImage(grayImage);
	int threshold1 = otsuThreshold(frame_gray);           //调用大津法函数
	threshold(grayImage, thresholdImage, threshold1, 255, CV_THRESH_BINARY_INV);
	namedWindow("【4.二值图】", WINDOW_NORMAL);
	imshow("【4.二值图】", thresholdImage);
	//-----------------------------------以上是我改动的---------------------------------------   

	//【6】中值滤波  
	medianBlur(thresholdImage, thresholdImage, 3);
	namedWindow("【5.滤波处理结果图】", WINDOW_NORMAL);
	imshow("【5.滤波处理结果图】", thresholdImage);



	//-----------------------------------【图像分割】---------------------------------------    
	//      描述：边缘检测，轮廓提取
	//-----------------------------------------------------------------------------------------------  

	//【1】获取轮廓  
	vector<vector<Point>> contours;
	findContours(thresholdImage,            //图像    
		contours,               //轮廓点     
		CV_RETR_EXTERNAL,       //获取轮廓的方法（这里获取外围轮廓）    
		CV_CHAIN_APPROX_NONE    //轮廓近似的方法（这里不近似，获取全部轮廓）  
	);
	//测试轮廓条数  
	cout << "轮廓：" << contours.size() << "条" << "\n";
	for (int i = 0; i<contours.size(); i++)
	{
		cout << "轮廓长：" << contours[i].size() << endl;
	}
	long cmin = 50;           //参数需视图片修改
	long cmax = 500;
	//vector<vector<Point>>::const_iterator itc = contours.begin();
	//while (itc != contours.end())
	//{
	//	if (itc->size() < cmin || itc->size() > cmax)
	//	{
	//		contours.erase(itc);
	//		itc = contours.begin();
	//	}
	//	else
	//		++itc;
	//}
	//cout << "有效轮廓：" << contours.size() << "条" << "\n" << endl;

	//【2】画出轮廓 
	Mat ContoursIm(thresholdImage.size(), CV_8U, Scalar(255));//制与原图同大小的画板  
	drawContours(ContoursIm, //画板  
		contours, //轮廓本身（指针）  
		-1,       //轮廓等级（这里画出所有轮廓）  
		Scalar(0),//颜色  
		-1);       //线粗，>=0时轮廓线粗；-1时既是CV_FILLED，填满  
	namedWindow("【6.ContoursIm】", WINDOW_NORMAL);
	imshow("【6.ContoursIm】", ContoursIm);


	//srcImage.release();
	//grayImage.release();
	//thresholdImage.release();
	//ContoursIm.release();

	}
}*/

int otsuThreshold(IplImage *frame)
{
#define GrayScale 256
	int width = frame->width;
	int height = frame->height;
	int pixelCount[GrayScale];
	float pixelPro[GrayScale];
	int i, j, pixelSum = width * height, threshold = 0;
	uchar* data = (uchar*)frame->imageData;  //指向像素数据的指针
	for (i = 0; i < GrayScale; i++)
	{
		pixelCount[i] = 0;
		pixelPro[i] = 0;
	}

	//统计灰度级中每个像素在整幅图像中的个数  
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			pixelCount[(int)data[i * width + j]]++;  //将像素值作为计数数组的下标
		}
	}

	//计算每个像素在整幅图像中的比例  
	float maxPro = 0.0;
	int kk = 0;
	for (i = 0; i < GrayScale; i++)
	{
		pixelPro[i] = (float)pixelCount[i] / pixelSum;
		if (pixelPro[i] > maxPro)
		{
			maxPro = pixelPro[i];
			kk = i;
		}
	}

	//遍历灰度级[0,255]  
	float w0, w1, u0tmp, u1tmp, u0, u1, u, deltaTmp, deltaMax = 0;
	for (i = 0; i < GrayScale; i++)     // i作为阈值
	{
		w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;
		for (j = 0; j < GrayScale; j++)
		{
			if (j <= i)   //背景部分  
			{
				w0 += pixelPro[j];
				u0tmp += j * pixelPro[j];
			}
			else   //前景部分  
			{
				w1 += pixelPro[j];
				u1tmp += j * pixelPro[j];
			}
		}

		u0 = u0tmp / w0;  //背景均值
		u1 = u1tmp / w1; //前景均值


		u = u0tmp + u1tmp;//总均值的定义
		deltaTmp = w0 * pow((u0 - u), 2) + w1 * pow((u1 - u), 2);//类间方差
		if (deltaTmp > deltaMax)
		{
			deltaMax = deltaTmp;
			threshold = i;
		}
	}

	return threshold;
}