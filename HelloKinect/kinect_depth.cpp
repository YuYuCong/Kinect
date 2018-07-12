///************************************************
//代码原作者：zouxy09
//来自：http://http://blog.csdn.net/zouxy09/article/details/8146266
//
//Kinect for Windows SDK1.8 + OpenCV2.4.9
//
//读取并显示深度图像
//
//************************************************/
//
//#include <windows.h>
//#include <iostream>
//#include <NuiApi.h>
//#include <opencv2/opencv.hpp>
//
//using namespace std;
//using namespace cv;
//
//int main(int argc, char *argv[])
//{
//	Mat image;
//	// 这里我们用灰度图来表述深度数据，越远的数据越暗
//	image.create(240, 320, CV_8UC1);
//
//	// 1. 初始化NUI，注意这里传入的参数有变化，为DEPTH
//	HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH);
//	if (FAILED(hr))
//	{
//		cout << "NuiInitialize failed" << endl;
//		return hr;
//	}
//
//	// 2. 定义事件句柄
//	// 创建读取下一帧的信号事件句柄，控制Kinect是否可以开始读取下一帧数据
//	HANDLE nextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//	HANDLE depthStreamHandle = NULL;  // 保存图像数据流的句柄，用以提取数据
//
//									  // 3. 打开Kinect设备的深度图信息通道，并用depthStreamHandle保存该流的句柄，以便以后读取
//	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_320x240,
//		0, 2, nextColorFrameEvent, &depthStreamHandle);
//	if (FAILED(hr))  // 判断读取是否正确
//	{
//		cout << "Could not open color image stream video" << endl;
//		NuiShutdown();
//		return hr;
//	}
//	namedWindow("depthImage", CV_WINDOW_AUTOSIZE);
//
//	// 4. 开始读取深度数据
//	while (true)
//	{
//		const NUI_IMAGE_FRAME * pImageFrame = NULL;
//
//		// 4.1 无限等待新的数据，等到后返回
//		if (WaitForSingleObject(nextColorFrameEvent, INFINITE) == 0)
//		{
//			// 4.2 从刚才打开数据流的流句柄中得到该帧数据，读取到的数据地址存于pImageFrame
//			hr = NuiImageStreamGetNextFrame(depthStreamHandle, 0, &pImageFrame);
//			if (FAILED(hr))  // 判断读取是否正确
//			{
//				cout << "Could not get depth image" << endl;
//				NuiShutdown();
//				return -1;
//			}
//
//			INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;
//			NUI_LOCKED_RECT LockedRect;
//
//			// 4.3 提取数据帧到LockedRect，它包括两个数据对象：Pitch每行字节数，pBits第一个字节地址
//			// 并锁定数据，这样当我们读取数据时，Kinect就不会去修改它
//			pTexture->LockRect(0, &LockedRect, NULL, 0);
//
//			// 4.4 确定获得的数据是否有效
//			if (LockedRect.Pitch != 0)
//			{
//				// 4.5 将数据转换为OpenCV的Mat格式
//				for (int i = 0; i < image.rows; ++i)
//				{
//					uchar *ptr = image.ptr<uchar>(i);  // 第i行的指针
//
//													   // 深度图像数据含有两种格式，这里像素的低12位表示一个深度值，高4位未使用
//													   // 注意这里需要进行转换，因为每个数据是2个字节，存储的同上面获取的颜色信息不一样
//					uchar *pBufferRun = (uchar*)(LockedRect.pBits) + i * LockedRect.Pitch;
//					USHORT *pBuffer = (USHORT*)pBufferRun;
//					for (int j = 0; j < image.cols; ++j)
//					{
//						ptr[j] = 255 - (uchar)(256 * pBuffer[j] / 0x0fff);  // 直接将数据归一化处理
//					}
//				}
//				imshow("depthImage", image);
//			}
//			else
//			{
//				cout << "Buffer length of received texture is bogus\r\n" << endl;
//			}
//
//			// 5. 这一帧已经处理完了，所以将其解锁
//			pTexture->UnlockRect(0);
//
//			// 6. 释放本帧数据，准备迎接下一帧
//			NuiImageStreamReleaseFrame(depthStreamHandle, pImageFrame);
//		}
//		if (cvWaitKey(20) == 27)
//		{
//			break;
//		}
//	}
//
//	// 7. 关闭NUI链接
//	return 0;
//}