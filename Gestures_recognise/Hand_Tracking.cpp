#include <windows.h>
#include <iostream> 
#include <NuiApi.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

bool tracked[NUI_SKELETON_COUNT] = { FALSE };
CvPoint skeletonPoint[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT] = { cvPoint(0,0) };
CvPoint colorPoint[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT] = { cvPoint(0,0) };

///2017.11.06���
vector<CvPoint> hand_center;

void getColorImage(HANDLE &colorEvent, HANDLE &colorStreamHandle, Mat &colorImage);
void getDepthImage(HANDLE &depthEvent, HANDLE &depthStreamHandle, Mat &depthImage);
void getSkeletonImage(HANDLE &skeletonEvent, Mat &skeletonImage, Mat &colorImage, Mat &depthImage);
void drawSkeleton(Mat &image, CvPoint pointSet[], int witchone);
void getTheContour(Mat &image, int whichone, Mat &mask);

///2017.11.06���
void drawHand(Mat&mask, Mat&hand, vector<CvPoint> &hand_center, int i);
///2017.11.19���
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
	///2017.11.06���
	Mat hand;    
	Mat grayImage, thresholdImage;//��ʱ������Ŀ��ͼ�Ķ���  
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
	hr = NuiSkeletonTrackingEnable(skeletonEvent, 0);//�򿪹��������¼�   
	if (hr != S_OK)
	{
		cout << "NuiSkeletonTrackingEnable failed" << endl;
		NuiShutdown();
		return hr;
	}

	///2017.11.06���
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
		//����ʹ��INFINITE��Ϊ�˱���û�м���skeletonEvent�������˴������colorimageƵ�������� 
		if (WaitForSingleObject(skeletonEvent, INFINITE) == 0)
			getSkeletonImage(skeletonEvent, skeletonImage, colorImage, depthImage);

		for (int i = 0; i < 6; i++)
		{
			if (tracked[i] == TRUE)
			{
				k ++;
				///2017.11.19���
				hand.setTo(0);

				mask.setTo(0);
				getTheContour(depthImage, i, mask);

				//-----------------------------------��ͼ���Ԥ����---------------------------------------    
				//      ������ͼ��Ķ��룬�ҶȻ�����ֵ�����˲�     
				//-----------------------------------------------------------------------------------------------      
				//��1������ԭʼͼ��Mat��������  
				if (mask.rows == 0 || hand_center.size() == 0)
				{
				}
				else if (   35 < hand_center[0].x && hand_center[0].x < (240 - 35)
					&& 35 < hand_center[0].y && hand_center[0].y < (320 - 35)   ) 
				{

					///2017.11.06���
					drawHand(mask, hand, hand_center, i);

					Mat srcImage = hand;  //����Ŀ¼��Ӧ����һ����Ϊ����.jpg���ز�ͼ    
					Mat grayImage, thresholdImage;//��ʱ������Ŀ��ͼ�Ķ���  

					//��2����ʾԭʼͼ    
					imshow("��1.ԭʼͼ��", srcImage);

					////��3����ȡ����Ȥ��  
					////-----------------------------------����������ӵ�---------------------------------------  
					////ʹ��cvsetImageROI����
					////cvsetImageROI(src��cvRect(x, y, width, height))
					////����x��yΪROI�������㣬width��heightΪ��͸ߣ�
					////��src��ȡ����Ȥ������ٶ�src����ͼ����ʱֻ�����ȡ�ĸ���Ȥ���д�����ͼ����ʱҪע����һ�㡣
					////ʹ�øú���ʱҪ��ͨ��cvResetImageROI()�����ͷ�ROI�������ͼ����д���ʱֻ��ROI������жԱȡ�
					////-----------------------------------����������ӵ�---------------------------------------    
					////Rect ROIim(300, 100, 600, 600);    //��������ͼƬ�޸�
					////Mat ROIsrc;
					////srcImage(ROIim).copyTo(ROIsrc);
					////namedWindow("��2.roiimage��", WINDOW_NORMAL);
					////imshow("��2.roiimage��", ROIsrc);

					//��4��תΪ�Ҷ�ͼ������ͼ��ƽ��    
					cvtColor(srcImage, grayImage, CV_BGR2GRAY);//ת����Ե�����ͼΪ�Ҷ�ͼ     
					imshow("��3.�Ҷ�ͼ��", grayImage);

					//��5��ͼ��Ķ�ֵ�������ô�� 
					//-----------------------------------�������ҸĶ���---------------------------------------    
					IplImage * frame_gray;
					frame_gray = &IplImage(grayImage);
					int threshold_number = otsuThreshold(frame_gray);           //���ô�򷨺���
					threshold(grayImage, thresholdImage, threshold_number, 255, CV_THRESH_BINARY);
					imshow("��4.��ֵͼ��", thresholdImage);
					//-----------------------------------�������ҸĶ���---------------------------------------   

					////��6����ֵ�˲�  
					//medianBlur(thresholdImage, thresholdImage, 3);
					//namedWindow("��5.�˲�������ͼ��", WINDOW_NORMAL);
					//imshow("��5.�˲�������ͼ��", thresholdImage);

					//-----------------------------------��ͼ��ָ---------------------------------------    
					//      ��������Ե��⣬������ȡ
					//-----------------------------------------------------------------------------------------------  
					if(k==30)
					{
						//waitKey(0);
						cout << k<<endl;
						//��1����ȡ����
						findContours(thresholdImage,            //ͼ��    
							contours,               //������     
							CV_RETR_EXTERNAL,       //��ȡ�����ķ����������ȡ��Χ������    
							CV_CHAIN_APPROX_NONE    //�������Ƶķ��������ﲻ���ƣ���ȡȫ��������  
						);
						//������������  
						if (contours.size() == 0) {}
						else {
							cout << "������" << contours.size() << "��" << endl;
							for (int i = 0; i < contours.size(); i++)
							{
								cout << "��������" << contours[i].size() << endl;
							}
							long cmin = 50;           //��������ͼƬ�޸�
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

							cout << "��Ч������" << contours.size() << "��" << endl;
							if (contours.size() == 0) {}
							else
							{
								for (int i = 0; i < contours.size(); i++)
								{
									cout << "��Ч��������" << contours[i].size() << endl;
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
			uchar *ptr = colorImage.ptr<uchar>(i);  //��i�е�ָ��					
													//ÿ���ֽڴ���һ����ɫ��Ϣ��ֱ��ʹ��uchar
			uchar *pBuffer = (uchar*)(LockedRect.pBits) + i * LockedRect.Pitch;
			for (int j = 0; j<colorImage.cols; j++)
			{
				ptr[3 * j] = pBuffer[4 * j];  //�ڲ�������4���ֽڣ�0-1-2��BGR����4������δʹ�� 
				ptr[3 * j + 1] = pBuffer[4 * j + 1];
				ptr[3 * j + 2] = pBuffer[4 * j + 2];
			}
		}
	}
	else
	{
		cout << "��׽ɫ��ͼ����ִ���" << endl;
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
					q.rgbGreen = q.rgbRed * 4;  //�������ó˵ķ�����������ԭ���ķ������Ա��ⲻ��������� 
					q.rgbBlue = q.rgbRed * 4;  //�����ں����getTheContour()�����ʹ�ã�������©һЩ��� 
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
		cout << "��׽���ͼ����ִ���" << endl;
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
		cout << "û���ҵ����ʵĹ���֡" << endl;
		return;
	}

	if (!bFoundSkeleton)
	{
		return;
	}

	NuiTransformSmooth(&skeletonFrame, NULL);//ƽ������֡,��������   
	skeletonImage.setTo(0);

	for (int i = 0; i < NUI_SKELETON_COUNT; i++)
	{
		if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&
			skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)
		{
			float fx, fy;

			for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++)//���е�����ת��Ϊ���ͼ������   
			{
				NuiTransformSkeletonToDepthImage(skeletonFrame.SkeletonData[i].SkeletonPositions[j], &fx, &fy);
				skeletonPoint[i][j].x = (int)fx;
				skeletonPoint[i][j].y = (int)fy;
			}

			for (int j = 0; j<NUI_SKELETON_POSITION_COUNT; j++)
			{
				if (skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED)//���ٵ�һ��������״̬��1û�б����ٵ���2���ٵ���3���ݸ��ٵ��Ĺ��Ƶ�   
				{
					LONG colorx, colory;
					NuiImageGetColorPixelCoordinatesFromDepthPixel ( NUI_IMAGE_RESOLUTION_640x480, 0,
						skeletonPoint[i][j].x, skeletonPoint[i][j].y, 0, &colorx, &colory);
					colorPoint[i][j].x = int(colorx);
					colorPoint[i][j].y = int(colory); //�洢����� 
					circle(colorImage, colorPoint[i][j], 4, cvScalar(0, 255, 255), 1, 8, 0);
					circle(skeletonImage, skeletonPoint[i][j], 3, cvScalar(0, 255, 255), 1, 8, 0);

					tracked[i] = TRUE;
				}
			}

			drawSkeleton(skeletonImage, skeletonPoint[i], i);
			drawSkeleton(colorImage, colorPoint[i], i);

			///2017.11.19���
			get_hand_center(skeletonImage, skeletonPoint[i], i);
		}
	}
}

void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone)
{
	CvScalar color;
	switch (whichone) //���ٲ�ͬ������ʾ��ͬ����ɫ 
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

	//����֫ 
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

	//����֫ 
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

	//����֫ 
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

	//����֫ 
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

//���ݸ�����������ݵĹ�ϵ����getDepthImage()�еģ�ȷ����ͬ�ĸ���Ŀ�� 
void getTheContour(Mat &image, int whichone, Mat &mask)
{
	for (int i = 0; i<image.rows; i++)
	{
		uchar *ptr = image.ptr<uchar>(i);
		uchar *ptrmask = mask.ptr<uchar>(i);
		for (int j = 0; j<image.cols; j++)
		{
			if (ptr[3 * j] == 0 && ptr[3 * j + 1] == 0 && ptr[3 * j + 2] == 0) //��Ϊ0��ʱ�����Ժ��� 
			{
				ptrmask[3 * j] = ptrmask[3 * j + 1] = ptrmask[3 * j + 2] = 0;
			}
			else if (ptr[3 * j] == 0 && ptr[3 * j + 1] == 0 && ptr[3 * j + 2] != 0)//IDΪ1��ʱ����ʾ��ɫ 
			{
				ptrmask[3 * j] = 0;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 0;
			}
			else if (ptr[3 * j] == 0 && ptr[3 * j + 1] != 0 && ptr[3 * j + 2] == 0)//IDΪ2��ʱ����ʾ��ɫ 
			{
				ptrmask[3 * j] = 0;
				ptrmask[3 * j + 1] = 0;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == ptr[3 * j + 1] && ptr[3 * j] == 4 * ptr[3 * j + 2])//IDΪ3��ʱ�� 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 0;
			}
			else if (4 * ptr[3 * j] == ptr[3 * j + 1] && ptr[3 * j + 1] == ptr[3 * j + 2])//IDΪ4��ʱ�� 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 0;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == 4 * ptr[3 * j + 1] && ptr[3 * j] == ptr[3 * j + 2])//IDΪ5��ʱ�� 
			{
				ptrmask[3 * j] = 0;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == 2 * ptr[3 * j + 1] && ptr[3 * j + 1] == ptr[3 * j + 2])//IDΪ6��ʱ�� 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 255;
				ptrmask[3 * j + 2] = 255;
			}
			else if (ptr[3 * j] == ptr[3 * j + 1] && ptr[3 * j] == ptr[3 * j + 2])//IDΪ7��ʱ�����IDΪ0��ʱ����ʾ��ɫ 
			{
				ptrmask[3 * j] = 255;
				ptrmask[3 * j + 1] = 0;
				ptrmask[3 * j + 2] = 0;
			}
			else
			{
				cout << "��������δ��룬˵������©����������ѯgetTheContour����" << endl;
			}
		}
	}
}


void get_hand_center(Mat &image, CvPoint pointSet[], int whichone)
{
	hand_center.push_back(pointSet[NUI_SKELETON_POSITION_HAND_RIGHT]);
}


///2017.11.06���
void drawHand(Mat&mask, Mat&hand, vector<CvPoint> &hand_center, int i)
{
	Rect rect;

	if (hand_center.size() == 0) 
	{
	}
	else{
		CvScalar color;
		switch (i) //���ٲ�ͬ������ʾ��ͬ����ɫ 
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
			///2017.11.19���
			if (hand_center.size() == 0)
			{
			}
			else {
				//circle(mask, hand_center[0], 5, Scalar(0, 0, 0), -1, 8, 0);
				
				//��ֹ����`
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
	//-----------------------------------��ͼ���Ԥ����---------------------------------------    
	//      ������ͼ��Ķ��룬�ҶȻ�����ֵ�����˲�     
	//-----------------------------------------------------------------------------------------------      
	//��1������ԭʼͼ��Mat��������  
	if (hand.rows == 0)
	{
	}
	else {
	Mat srcImage = hand;  //����Ŀ¼��Ӧ����һ����Ϊ����.jpg���ز�ͼ    
	Mat grayImage, thresholdImage;//��ʱ������Ŀ��ͼ�Ķ���  

	//��2����ʾԭʼͼ    
	namedWindow("��1.ԭʼͼ��", WINDOW_NORMAL);
	imshow("��1.ԭʼͼ��", srcImage);

	//��3����ȡ����Ȥ��  
	//-----------------------------------����������ӵ�---------------------------------------  
	//ʹ��cvsetImageROI����
	//cvsetImageROI(src��cvRect(x, y, width, height))
	//����x��yΪROI�������㣬width��heightΪ��͸ߣ�
	//��src��ȡ����Ȥ������ٶ�src����ͼ����ʱֻ�����ȡ�ĸ���Ȥ���д�����ͼ����ʱҪע����һ�㡣
	//ʹ�øú���ʱҪ��ͨ��cvResetImageROI()�����ͷ�ROI�������ͼ����д���ʱֻ��ROI������жԱȡ�
	//-----------------------------------����������ӵ�---------------------------------------    
	//Rect ROIim(300, 100, 600, 600);    //��������ͼƬ�޸�
	//Mat ROIsrc;
	//srcImage(ROIim).copyTo(ROIsrc);
	//namedWindow("��2.roiimage��", WINDOW_NORMAL);
	//imshow("��2.roiimage��", ROIsrc);

	//��4��תΪ�Ҷ�ͼ������ͼ��ƽ��    
	cvtColor(srcImage, grayImage, CV_BGR2GRAY);//ת����Ե�����ͼΪ�Ҷ�ͼ     
	namedWindow("��3.�Ҷ�ͼ��", WINDOW_NORMAL);
	imshow("��3.�Ҷ�ͼ��", grayImage);

	//��5��ͼ��Ķ�ֵ�������ô�� 
	//-----------------------------------�������ҸĶ���---------------------------------------    
	IplImage * frame_gray;
	frame_gray = &IplImage(grayImage);
	int threshold1 = otsuThreshold(frame_gray);           //���ô�򷨺���
	threshold(grayImage, thresholdImage, threshold1, 255, CV_THRESH_BINARY_INV);
	namedWindow("��4.��ֵͼ��", WINDOW_NORMAL);
	imshow("��4.��ֵͼ��", thresholdImage);
	//-----------------------------------�������ҸĶ���---------------------------------------   

	//��6����ֵ�˲�  
	medianBlur(thresholdImage, thresholdImage, 3);
	namedWindow("��5.�˲�������ͼ��", WINDOW_NORMAL);
	imshow("��5.�˲�������ͼ��", thresholdImage);



	//-----------------------------------��ͼ��ָ---------------------------------------    
	//      ��������Ե��⣬������ȡ
	//-----------------------------------------------------------------------------------------------  

	//��1����ȡ����  
	vector<vector<Point>> contours;
	findContours(thresholdImage,            //ͼ��    
		contours,               //������     
		CV_RETR_EXTERNAL,       //��ȡ�����ķ����������ȡ��Χ������    
		CV_CHAIN_APPROX_NONE    //�������Ƶķ��������ﲻ���ƣ���ȡȫ��������  
	);
	//������������  
	cout << "������" << contours.size() << "��" << "\n";
	for (int i = 0; i<contours.size(); i++)
	{
		cout << "��������" << contours[i].size() << endl;
	}
	long cmin = 50;           //��������ͼƬ�޸�
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
	//cout << "��Ч������" << contours.size() << "��" << "\n" << endl;

	//��2���������� 
	Mat ContoursIm(thresholdImage.size(), CV_8U, Scalar(255));//����ԭͼͬ��С�Ļ���  
	drawContours(ContoursIm, //����  
		contours, //��������ָ�룩  
		-1,       //�����ȼ������ﻭ������������  
		Scalar(0),//��ɫ  
		-1);       //�ߴ֣�>=0ʱ�����ߴ֣�-1ʱ����CV_FILLED������  
	namedWindow("��6.ContoursIm��", WINDOW_NORMAL);
	imshow("��6.ContoursIm��", ContoursIm);


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
	uchar* data = (uchar*)frame->imageData;  //ָ���������ݵ�ָ��
	for (i = 0; i < GrayScale; i++)
	{
		pixelCount[i] = 0;
		pixelPro[i] = 0;
	}

	//ͳ�ƻҶȼ���ÿ������������ͼ���еĸ���  
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			pixelCount[(int)data[i * width + j]]++;  //������ֵ��Ϊ����������±�
		}
	}

	//����ÿ������������ͼ���еı���  
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

	//�����Ҷȼ�[0,255]  
	float w0, w1, u0tmp, u1tmp, u0, u1, u, deltaTmp, deltaMax = 0;
	for (i = 0; i < GrayScale; i++)     // i��Ϊ��ֵ
	{
		w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;
		for (j = 0; j < GrayScale; j++)
		{
			if (j <= i)   //��������  
			{
				w0 += pixelPro[j];
				u0tmp += j * pixelPro[j];
			}
			else   //ǰ������  
			{
				w1 += pixelPro[j];
				u1tmp += j * pixelPro[j];
			}
		}

		u0 = u0tmp / w0;  //������ֵ
		u1 = u1tmp / w1; //ǰ����ֵ


		u = u0tmp + u1tmp;//�ܾ�ֵ�Ķ���
		deltaTmp = w0 * pow((u0 - u), 2) + w1 * pow((u1 - u), 2);//��䷽��
		if (deltaTmp > deltaMax)
		{
			deltaMax = deltaTmp;
			threshold = i;
		}
	}

	return threshold;
}