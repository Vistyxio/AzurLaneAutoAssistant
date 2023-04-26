#include<iostream>
#include<fstream>
#include<cstdlib>
#include<ctime>
#include<vector>
#include<thread>
#include<algorithm>
#include<opencv2/opencv.hpp>
#include<Windows.h>

using namespace cv;
using namespace std;

//#define TEST
#define MAKELIST

int srcheight, srcwidth;
int srcroiheight, srcroiwidth;
bool ifget = true;
vector<Point2d> windowSize;
struct Aclick {
	string str;
	double x1, y1, x2, y2;
};
struct Alist {
	string n;
	vector<Aclick> list;
};
vector<Alist> AllList; //�洢ÿһ������

Mat hwnd2mat(HWND hwnd) {
	HDC hwindowDC, hwindowCompatibleDC;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER  bi{};
	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);
	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	RECT rect = { 0 };

	if (hwnd == NULL) {
		HDC hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
		srcheight = GetDeviceCaps(hdc, VERTRES);
		srcwidth = GetDeviceCaps(hdc, HORZRES);
	} else {
		GetWindowRect(hwnd, &rect);
		srcwidth = rect.right - rect.left;
		srcheight = rect.bottom - rect.top;
	}

	src.create(srcheight, srcwidth, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, srcwidth, srcheight);

	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = srcwidth;
	bi.biHeight = -srcheight;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, srcwidth, srcheight, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, srcheight, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow
	// avoid memory leak
	DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);
	return src;
}

inline int random(int num, int offset) { //�������
	return num + (rand() % 2 ? -1 : 1) * (rand() % (offset + 1)); //ֻ�������ͷ����޷�ȡ��
}

struct MatchResult {
	bool isFind = false;
	Point2f pos;
	double Val = 0.;
};

MatchResult findTemplate(Mat& _src, Mat& _mytemplate, double thresholdVal) {
	MatchResult Result;

	int result_cols = _src.cols - _mytemplate.cols + 1;
	int result_rows = _src.rows - _mytemplate.rows + 1;
	Mat result(result_cols, result_rows, CV_32FC1);

	//CV_TM_SQDIFF 0
	//CV_TM_SQDIFF_NORMED 1
	//CV_TM_CCORR 2
	//CV_TM_CCORR_NORMED 3
	//CV_TM_CCOEFF 4
	//CV_TM_CCOEFF_NORMED 5
	matchTemplate(_src, _mytemplate, result, 1);//����ʹ�õ�ƥ���㷨�Ǳ�׼ƽ����ƥ�� method=CV_TM_SQDIFF_NORMED����ֵԽСƥ���Խ��
	normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

	//imshow("result", result);

	double minVal, maxVal;
	Point minLoc, maxLoc, matchLoc;

	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

	if (minVal == maxVal) return Result;

	//abs(minVal) < 1e-7
	if (abs(minVal) < thresholdVal) { //
		cout << abs(minVal) << " < " << thresholdVal << endl;
		matchLoc = minLoc;

		Result.isFind = true;
		Result.pos.x = matchLoc.x + _mytemplate.cols / 2.;
		Result.pos.y = matchLoc.y + _mytemplate.rows / 2.;
		Result.Val = minVal;

		//circle(_src, Result.pos, 5, Scalar(255, 0, 0), 2, LINE_AA, 0);
	}


	return Result;
}

/*void OnMouse(int event, int x, int y, int flags, void*) { //�ص�����
	if (event == EVENT_LBUTTONDOWN) {
		//��Ҫע����ǣ����ں���ΪX�ᣬ����ΪY�ᣬ������(x, y)��
		//�����������ǰ���(row, col)���ʴ����еĵ�(x, y)��Ӧ��ֵ�ھ�����Ϊ(y, x)��
		windowSize.emplace_back(Point2d(2 * x, 2 * y)); //ǰ��resize��С����������Ҫ��Ӧ�Ŵ�
	}
}*/

#ifdef MAKELIST
void TESTonMouse(int event, int x, int y, int flags, void*) {
	if (event == EVENT_LBUTTONDOWN) {
		cout << x / (windowSize[1].x - windowSize[0].x) << ", " << y / (windowSize[1].y - windowSize[0].y) << ", " << endl;
	}
}
#endif

inline void drag(double axwratio, double ayhratio, double bxwratio, double byhratio) { //�����ڱ����϶�
	mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
		(axwratio * srcroiwidth + windowSize[0].x) * 65536 / srcwidth,
		(ayhratio * srcroiheight + windowSize[0].y) * 65536 / srcheight,
		0, 0);
	Sleep(100);
	mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	Sleep(100);
	mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
		(bxwratio * srcroiwidth + windowSize[0].x) * 65536 / srcwidth,
		(byhratio * srcroiheight + windowSize[0].y) * 65536 / srcheight,
		0, 0);
	Sleep(100);
	mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	Sleep(200);
	return;
}

inline void click(double axwratio, double ayhratio, double bxwratio, double byhratio) { //���ڱ�����ʽ����
	mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
		random((bxwratio + axwratio) / 2. * srcroiwidth + windowSize[0].x,
			(bxwratio - axwratio) / 2. * srcroiwidth * 0.85) * 65536 / srcwidth,
		random((byhratio + ayhratio) / 2. * srcroiheight + windowSize[0].y,
			(byhratio - ayhratio) / 2. * srcroiheight * 0.85) * 65536 / srcheight,
		0, 0);
	mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	return;
}

inline void click(Point2f p, double offsetX, double offsetY) { //������ʽ����
	mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 
		random(p.x + windowSize[0].x, offsetX * 0.85) * 65536 / srcwidth, 
		random(p.y + windowSize[0].y, offsetY * 0.85) * 65536 / srcheight, 
		0, 0);
	mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	return;
}

inline int DoOnce(string str, double axwratio, double ayhratio, double bxwratio, double byhratio) { //ִ�е�������
	cout << str;
	click(axwratio, ayhratio, bxwratio, byhratio);
	cout << "......Done" << endl << endl;
	Sleep(random(1500, 300));
	return 0;
}

int DoAsList(Alist A) { //�������嵥ִ��
	cout << A.n << endl;
	for (auto& i : A.list) { //ִ��ÿһ������
		DoOnce(i.str, i.x1, i.y1, i.x2, i.y2); //ִ�е�������
	}
	return 0;
}

void getSize(LPCWSTR name) { //���̺߳���ʵʱ��ȡ����λ�úʹ�С
	HWND h = NULL;
	RECT rect = { 0 };
	windowSize.clear();
	windowSize.emplace_back(Point2d(0., 0.));
	windowSize.emplace_back(Point2d(0., 0.));
	HDC hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	srcheight = GetDeviceCaps(hdc, VERTRES);
	srcwidth = GetDeviceCaps(hdc, HORZRES);
	
	while (true) {
		h = FindWindow(NULL, name);
		if (h == 0) {
			cout << "thread hwnd = 0" << endl;
			ifget = false;
			return;
		}
		GetWindowRect(h, &rect);
		windowSize[0].x = (rect.left < 0) ? 0 : rect.left;
		windowSize[0].y = (rect.top < 0) ? 0 : rect.top;
		windowSize[1].x = (rect.right > srcwidth) ? srcwidth : rect.right;
		windowSize[1].y = (rect.bottom > srcheight) ? srcheight : rect.bottom;

		srcroiwidth = windowSize[1].x - windowSize[0].x;
		srcroiheight = windowSize[1].y - windowSize[0].y;

		Sleep(100);
	}
	return;
}

int PromptDetect() { //������ʾ�ļ��
	
	while (true) {
		if (!ifget) {
			cout << "thread error" << endl;
			return -1;
		}
		Mat src = hwnd2mat(NULL);
		if (src.empty()) {
			cout << "src is empty" << endl;
			return -1;
		}
		Mat org = src(Rect(windowSize[0], windowSize[1]));
		vector<Mat> chann;
		split(org, chann); //0B 1G 2R
		Mat orgs = chann[0] - chann[1];
		GaussianBlur(orgs, orgs, Size(17, 17), 0, 0);
		//imshow("orgs", orgs);
		//waitKey(0);
		threshold(orgs, orgs, 50, 255, THRESH_BINARY);
		morphologyEx(orgs, orgs, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(13, 13), Point(-1, -1)));

		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(orgs, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


		Mat draw = Mat::zeros(orgs.size(), CV_8UC3);

		
		struct Q {
			Point2f c;
			float w = 0., h = 0.;
		} temp;
		vector<Q> center;
		Point2f confirm;

		for (auto& i : contours) {
			RotatedRect rects = minAreaRect(i);
			if (contourArea(i) / (rects.size.width * rects.size.height) < 0.9) {
				continue;
			}
			if (contourArea(i) < 300) {
				continue;
			}
			temp.c = rects.center, temp.w = rects.size.width, temp.h = rects.size.height;
			center.emplace_back(temp);
		}

		if (center.size() == 4) { //��������
			cout << "��������" << endl;
			sort(center.begin(), center.end(), [](Q x, Q y) {return (x.c.x * x.c.x + x.c.y * x.c.y) > (y.c.x * y.c.x + y.c.y * y.c.y); });
			Q confirm = center[0];
			//click(confirm.c, confirm.w / 2., confirm.h / 2.);
			DoAsList(AllList[1]);

		} else {
			sort(center.begin(), center.end(), [](Q x, Q y) {return (x.c.x * x.c.x + x.c.y * x.c.y) > (y.c.x * y.c.x + y.c.y * y.c.y); });
			Q confirm = center[0];
		}

		

		for (size_t i = 0; i < contours.size(); i++) {

			drawContours(draw, contours, i, Scalar(0, 255, 0), 1, LINE_AA, hierarchy, 0);
			//putText(draw, to_string(contourArea(contours[i])), minAreaRect(contours[i]).center, 1, 2, Scalar(0, 0, 255), 1, LINE_AA);
		}

		for (size_t i = 0; i < center.size(); i++) {
			putText(draw, to_string(i), center[i].c, 1, 2, Scalar(0, 0, 255), 1, LINE_AA);
		}

		//putText(draw, "000", confirm, 5, 1, Scalar(255, 255, 255), 2, LINE_AA);

		imshow("", draw);
		waitKey(0);


		Sleep(100);
	}

	
	return 0;
}

int newMap() {

	/*for (int i = 0; i < 3; i++) {
		drag(0.3, 0.3, 0.8, 0.8);
	}*/

	while (true) {
		break;
		if (!ifget) {
			cout << "thread error" << endl;
			return -1;
		}
		Mat src = hwnd2mat(NULL);
		if (src.empty()) {
			cout << "src is empty" << endl;
			return -1;
		}
		Mat src_roi = src(Rect(windowSize[0], windowSize[1])); //��ȡ��Ϸ����

		Mat sss;
		cvtColor(src_roi, sss, COLOR_BGR2GRAY);
		//threshold(sss, sss, 90, 255, THRESH_BINARY_INV);
		adaptiveThreshold(sss, sss, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 13, -10);
		//morphologyEx(sss, sss, MORPH_ERODE, getStructuringElement(MORPH_RECT, Size(3, 3)));

		/*vector<Vec4i> lines;
		HoughLinesP(sss, lines, 1, CV_PI / 180, 80, 50, 10);

		Mat draw = Mat::zeros(sss.size(), CV_8UC3);

		for (size_t i = 0; i < lines.size(); i++) {
			Vec4i l = lines[i];
			line(draw, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(rand() % 256, rand() % 256, rand() % 256), 1, LINE_AA);
		}*/
		Mat mask = Mat::zeros(src_roi.size(), CV_8UC1);
		rectangle(mask, Point(0.11 * srcroiwidth, 0.26 * srcroiheight), Point(0.91 * srcroiwidth, 0.865 * srcroiheight), Scalar::all(255), -1, LINE_AA, 0);
		//imshow("mask", mask);
		//waitKey(0);
		rectangle(src_roi, Point(0.11 * srcroiwidth, 0.26 * srcroiheight), Point(0.91 * srcroiwidth, 0.865 * srcroiheight), Scalar::all(255), 2, LINE_AA, 0);

		vector<Point2f> corners;
		goodFeaturesToTrack(sss, corners, 20, 0.01, 15, mask, 9, false, 0.04); //���Ͻ�20

		TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::MAX_ITER, 40, 0.01);	
		cornerSubPix(sss, corners, Size(7, 7), Size(-1, -1), criteria);
		Mat blackwhite = Mat::zeros(sss.size(), CV_8UC1);
		Mat black1, black2, black3, black;

		for (size_t i = 0; i < corners.size(); i++) {
			circle(src_roi, corners[i], 4, Scalar(rand() % 256, rand() % 256, rand() % 256), 2, LINE_AA);
			circle(blackwhite, corners[i], 3, Scalar::all(255), -1, LINE_AA);
		}
		morphologyEx(blackwhite, blackwhite, MORPH_DILATE, getStructuringElement(MORPH_RECT, Size(19, 19)));
		//morphologyEx(blackwhite, black1, MORPH_ERODE, getStructuringElement(MORPH_RECT, Size(17, 17)));
		//morphologyEx(black1, black2, MORPH_ERODE, getStructuringElement(MORPH_RECT, Size(7, 7)));
		//morphologyEx(black2, black3, MORPH_DILATE, getStructuringElement(MORPH_RECT, Size(7, 7)));

		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(blackwhite, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		vector<Point> TrackCenter;

		for (auto& i : contours) {
			RotatedRect rects = minAreaRect(i);
			if (contourArea(i) > 670) {
				continue;
			}
			TrackCenter.emplace_back(rects.center);
		}

		//Mat TrackPoint = Mat::zeros(src_roi.size(), CV_8UC1);

		/*for (auto& i : TrackCenter) {
			circle(src_roi, i, 8, Scalar(0, 0, 255), 3, LINE_AA);
			circle(TrackPoint, i, 3, Scalar::all(255), -1, LINE_AA);
		}*/

		//�ҵ�ͼ���Ͻǽǵ�
		sort(TrackCenter.begin(), TrackCenter.end(), [](Point x, Point y) {return (x.x * x.x + x.y * x.y) < (y.x * y.x + y.y * y.y); });
		Point confirm = TrackCenter[0];

		circle(src_roi, confirm, 5, Scalar::all(255), -1, LINE_AA);

		//imshow("sss", sss);
		imshow("src_roi", src_roi);
		imshow("blackwhite", blackwhite);
		//imshow("TrackPoint", TrackPoint);
		//imshow("black1", black1);
		//imshow("black2", black2);
		//imshow("black3", black3);
		//imshow("black", black);
		//imshow("draw", draw);
		//moveWindow("sss", 0, 0); //�̶�imshow���������Ͻ�
		waitKey(0);

		//system("pause");
		
		break;
		Sleep(100);
	}

	while (true) {
		if (!ifget) {
			cout << "thread error" << endl;
			return -1;
		}
		Mat src = hwnd2mat(NULL);
		if (src.empty()) {
			cout << "src is empty" << endl;
			return -1;
		}
		Mat src_roi = src(Rect(windowSize[0], windowSize[1])); //��ȡ��Ϸ����
		Mat sss;
		cvtColor(src_roi, sss, COLOR_BGR2GRAY);
		adaptiveThreshold(sss, sss, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 13, -10);
		Mat mask = Mat::zeros(src_roi.size(), CV_8UC1);
		rectangle(mask, Point(0.11 * srcroiwidth, 0.26 * srcroiheight), Point(0.91 * srcroiwidth, 0.865 * srcroiheight), Scalar::all(255), -1, LINE_AA, 0);
		rectangle(src_roi, Point(0.11 * srcroiwidth, 0.26 * srcroiheight), Point(0.91 * srcroiwidth, 0.865 * srcroiheight), Scalar::all(255), 2, LINE_AA, 0);

		vector<Point2f> corners;
		goodFeaturesToTrack(sss, corners, 100, 0.01, 15, mask, 9, false, 0.04); //���»���ȷ������

		TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::MAX_ITER, 40, 0.01);
		cornerSubPix(sss, corners, Size(7, 7), Size(-1, -1), criteria);
		Mat blackwhite = Mat::zeros(sss.size(), CV_8UC1);
		Mat black1, black2, black3, black;

		for (size_t i = 0; i < corners.size(); i++) {
			circle(src_roi, corners[i], 4, Scalar(rand() % 256, rand() % 256, rand() % 256), 2, LINE_AA);
			circle(blackwhite, corners[i], 3, Scalar::all(255), -1, LINE_AA);
		}
		morphologyEx(blackwhite, blackwhite, MORPH_DILATE, getStructuringElement(MORPH_RECT, Size(19, 19)));

		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(blackwhite, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		vector<Point> TrackCenter;

		for (auto& i : contours) {
			RotatedRect rects = minAreaRect(i);
			if (contourArea(i) > 670) {
				continue;
			}
			TrackCenter.emplace_back(rects.center);
		}

		imshow("src_roi", src_roi);
		imshow("blackwhite", blackwhite);
		waitKey(0);

		Sleep(100);
	}




	return 0;
}

int oldMeta() { //�����ű�
	//DoAsList(AllList[2]);

	MatchResult result0, result1, result2, result3;

	Mat oldMetaOK = imread("oldMetaOK.png");
	Mat battlePAUSE = imread("battlePAUSE.png");
	Mat oldMetaAGAIN = imread("oldMetaAGAIN.png");
	Mat oldMetaCOLLECT = imread("oldMetaCOLLECT.png");

	resize(oldMetaOK, oldMetaOK, Size(0, 0), double(srcroiwidth) / 1920., double(srcroiheight) / 1080., INTER_CUBIC);
	resize(battlePAUSE, battlePAUSE, Size(0, 0), double(srcroiwidth) / 1920., double(srcroiheight) / 1080., INTER_CUBIC);
	resize(oldMetaAGAIN, oldMetaAGAIN, Size(0, 0), double(srcroiwidth) / 1920., double(srcroiheight) / 1080., INTER_CUBIC);
	resize(oldMetaCOLLECT, oldMetaCOLLECT, Size(0, 0), double(srcroiwidth) / 1920., double(srcroiheight) / 1080., INTER_CUBIC);

	cvtColor(oldMetaOK, oldMetaOK, COLOR_BGR2GRAY);
	cvtColor(battlePAUSE, battlePAUSE, COLOR_BGR2GRAY);
	cvtColor(oldMetaAGAIN, oldMetaAGAIN, COLOR_BGR2GRAY);
	cvtColor(oldMetaCOLLECT, oldMetaCOLLECT, COLOR_BGR2GRAY);

	//threshold(oldMetaOK, oldMetaOK, 150, 255, THRESH_BINARY);
	//threshold(oldMetaAGAIN, oldMetaAGAIN, 150, 255, THRESH_BINARY);
	//threshold(oldMetaCOLLECT, oldMetaCOLLECT, 150, 255, THRESH_BINARY);

	while (true) {
		if (!ifget) {
			cout << "thread error" << endl;
			return -1;
		}
		Mat src = hwnd2mat(NULL);
		if (src.empty()) {
			cout << "src is empty" << endl;
			return -1;
		}
		Mat src_roi = src(Rect(windowSize[0], windowSize[1])); //��ȡ����

		cvtColor(src_roi, src_roi, COLOR_BGR2GRAY);
		//threshold(src_roi, src_roi, 150, 255, THRESH_BINARY);

		result0 = findTemplate(src_roi, battlePAUSE, 4e-9);
		if (result0.isFind) {
			cout << "continue" << endl;
			Sleep(2000);
			continue;
		}

		result1 = findTemplate(src_roi, oldMetaOK, 8e-10);
		if (result1.isFind) {
			DoOnce("�����ű�ս�����", 0.776708, 0.872581, 0.910491, 0.946774);
		}

		result2 = findTemplate(src_roi, oldMetaAGAIN, 2e-9);
		if (result2.isFind) {
			DoOnce("�����ű꿪ʼ��ս1", 0.822907, 0.872581, 0.976901, 0.954839);
			DoOnce("�����ű꿪ʼ��ս2", 0.809432, 0.835484, 0.962464, 0.922581);
			Sleep(5000);
		}

		result3 = findTemplate(src_roi, oldMetaCOLLECT, 1e-9);
		if (result3.isFind) {
			DoOnce("�����ű���ȡ����", 0.820019, 0.869355, 0.977863, 0.958065);
			DoOnce("�����ű���ȡ����", 0.820019, 0.869355, 0.977863, 0.958065);
			DoOnce("��������ű�", 0.550529, 0.269355, 0.770934, 0.775806);
			DoOnce("�����ű��ٴν���", 0.428296, 0.203226, 0.583253, 0.704839);
		}

		Sleep(500);
	}
	

	return 0;
}

int main() { //������������������������������������������������������������������������������
	srand((unsigned)time(NULL));

	/*while (true) { //ͨ���ص�������ȡ���ڵ����ϽǺ����½�
		Mat src = hwnd2mat(hwnd);
		Mat resizesrc;
		resize(src, resizesrc, Size(src.size().width / 2, src.size().height / 2.));
		if (src.empty()) {
			cout << "src is empty" << endl;
			return -1;
		}
		windowSize.clear();
		imshow("ѡȡ�������ߴ������Ϻ����½�", resizesrc);
		moveWindow("ѡȡ�������ߴ������Ϻ����½�", 0, 0);
		setMouseCallback("ѡȡ�������ߴ������Ϻ����½�", OnMouse);
		waitKey(0);
		if (windowSize.size() == 2) {
			destroyAllWindows();
			break;
		}
	}
	srcroiwidth = windowSize[1].x - windowSize[0].x;
	srcroiheight = windowSize[1].y - windowSize[0].y;
	*/

	thread getSizet(getSize, TEXT("��������")); //���߳�
	getSizet.detach(); //�̷߳���
	Sleep(500); //�ȴ����߳�ִ��

	//SetForegroundWindow(FindWindow(NULL, TEXT("��������"))); //������������Ϸ��������������
	//Sleep(100);

	vector<Aclick> tempVQ;
	ifstream fs;
	fs.open("list.txt", ios::in);
	if (!fs.is_open()) {
		std::cout << "no txt file" << endl;
		return -1;
	}
	string str, n;
	double temp1, temp2, temp3, temp4;
	while (fs >> str) { //���ļ���ȡ����
		if ((str[0] >= 'A') && (str[0] <= 'Z')) {
			if (tempVQ.size() != 0) {
				AllList.emplace_back(Alist{ n, tempVQ });
				tempVQ.clear();
			}
			n = str;
			fs >> str;
			fs >> temp1;
			fs >> temp2;
			fs >> temp3;
			fs >> temp4;
			tempVQ.emplace_back(Aclick{ str, temp1, temp2, temp3, temp4 });

		} else {
			fs >> temp1;
			fs >> temp2;
			fs >> temp3;
			fs >> temp4;
			tempVQ.emplace_back(Aclick{ str, temp1, temp2, temp3, temp4 });
		}
	}
	fs.close();

	cout << "-1.����" << endl;
	cout << "1.�Զ�ˢ�����ű�" << endl;
	int choice;
	cin >> choice;

	SetForegroundWindow(FindWindow(NULL, TEXT("��������"))); //������������Ϸ��������������
	Sleep(100);

	if (choice == -1) {

	}else if (choice == 1) {
		if (oldMeta() == -1) {
			cout << "oldMeta error" << endl;
			return -1;
		}
	}

	//thread fullDockt(fullDock);
	//fullDockt.detach();
	//Sleep(500);

	//system("pause");

	//newMap();
	//PromptDetect();
	system("pause");

	while (true) { //��ѭ����ѭ����ѭ����ѭ����ѭ����ѭ����ѭ����ѭ����ѭ����ѭ��
		if (!ifget) {
			cout << "thread error" << endl;
			return -1;
		}
		Mat src = hwnd2mat(NULL);
		if (src.empty()) {
			cout << "src is empty" << endl;
			return -1;
		}
		
#ifdef TEST
		Mat src_roi = src(Rect(windowSize[0], windowSize[1])); //��ȡ����
		imshow("src_roi", src_roi);
		moveWindow("src_roi", 0, 0); //�̶�imshow���������Ͻ�
		waitKey(100);
#endif

#ifdef MAKELIST
		Mat src_roi = src(Rect(windowSize[0], windowSize[1])); //��ȡ��Ϸ����
		imshow("src_roi", src_roi);
		moveWindow("src_roi", 0, 0); //�̶�imshow���������Ͻ�
		setMouseCallback("src_roi", TESTonMouse);
		waitKey(0);
#endif

#ifndef TEST
#ifndef MAKELIST
		DoAsList(AllList[0]);
		break;
#endif
#endif
		
		

		Sleep(100);
	}

	
	return 0;
}