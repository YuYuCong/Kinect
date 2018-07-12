///************************************************
//����ԭ���ߣ�zouxy09
//���ԣ�http://http://blog.csdn.net/zouxy09/article/details/8146266
//
//Kinect for Windows SDK1.8 + OpenCV2.4.9
//
//��ȡ����ʾ���ͼ��
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
//	// ���������ûҶ�ͼ������������ݣ�ԽԶ������Խ��
//	image.create(240, 320, CV_8UC1);
//
//	// 1. ��ʼ��NUI��ע�����ﴫ��Ĳ����б仯��ΪDEPTH
//	HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH);
//	if (FAILED(hr))
//	{
//		cout << "NuiInitialize failed" << endl;
//		return hr;
//	}
//
//	// 2. �����¼����
//	// ������ȡ��һ֡���ź��¼����������Kinect�Ƿ���Կ�ʼ��ȡ��һ֡����
//	HANDLE nextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//	HANDLE depthStreamHandle = NULL;  // ����ͼ���������ľ����������ȡ����
//
//									  // 3. ��Kinect�豸�����ͼ��Ϣͨ��������depthStreamHandle��������ľ�����Ա��Ժ��ȡ
//	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_320x240,
//		0, 2, nextColorFrameEvent, &depthStreamHandle);
//	if (FAILED(hr))  // �ж϶�ȡ�Ƿ���ȷ
//	{
//		cout << "Could not open color image stream video" << endl;
//		NuiShutdown();
//		return hr;
//	}
//	namedWindow("depthImage", CV_WINDOW_AUTOSIZE);
//
//	// 4. ��ʼ��ȡ�������
//	while (true)
//	{
//		const NUI_IMAGE_FRAME * pImageFrame = NULL;
//
//		// 4.1 ���޵ȴ��µ����ݣ��ȵ��󷵻�
//		if (WaitForSingleObject(nextColorFrameEvent, INFINITE) == 0)
//		{
//			// 4.2 �ӸղŴ���������������еõ���֡���ݣ���ȡ�������ݵ�ַ����pImageFrame
//			hr = NuiImageStreamGetNextFrame(depthStreamHandle, 0, &pImageFrame);
//			if (FAILED(hr))  // �ж϶�ȡ�Ƿ���ȷ
//			{
//				cout << "Could not get depth image" << endl;
//				NuiShutdown();
//				return -1;
//			}
//
//			INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;
//			NUI_LOCKED_RECT LockedRect;
//
//			// 4.3 ��ȡ����֡��LockedRect���������������ݶ���Pitchÿ���ֽ�����pBits��һ���ֽڵ�ַ
//			// ���������ݣ����������Ƕ�ȡ����ʱ��Kinect�Ͳ���ȥ�޸���
//			pTexture->LockRect(0, &LockedRect, NULL, 0);
//
//			// 4.4 ȷ����õ������Ƿ���Ч
//			if (LockedRect.Pitch != 0)
//			{
//				// 4.5 ������ת��ΪOpenCV��Mat��ʽ
//				for (int i = 0; i < image.rows; ++i)
//				{
//					uchar *ptr = image.ptr<uchar>(i);  // ��i�е�ָ��
//
//													   // ���ͼ�����ݺ������ָ�ʽ���������صĵ�12λ��ʾһ�����ֵ����4λδʹ��
//													   // ע��������Ҫ����ת������Ϊÿ��������2���ֽڣ��洢��ͬ�����ȡ����ɫ��Ϣ��һ��
//					uchar *pBufferRun = (uchar*)(LockedRect.pBits) + i * LockedRect.Pitch;
//					USHORT *pBuffer = (USHORT*)pBufferRun;
//					for (int j = 0; j < image.cols; ++j)
//					{
//						ptr[j] = 255 - (uchar)(256 * pBuffer[j] / 0x0fff);  // ֱ�ӽ����ݹ�һ������
//					}
//				}
//				imshow("depthImage", image);
//			}
//			else
//			{
//				cout << "Buffer length of received texture is bogus\r\n" << endl;
//			}
//
//			// 5. ��һ֡�Ѿ��������ˣ����Խ������
//			pTexture->UnlockRect(0);
//
//			// 6. �ͷű�֡���ݣ�׼��ӭ����һ֡
//			NuiImageStreamReleaseFrame(depthStreamHandle, pImageFrame);
//		}
//		if (cvWaitKey(20) == 27)
//		{
//			break;
//		}
//	}
//
//	// 7. �ر�NUI����
//	return 0;
//}