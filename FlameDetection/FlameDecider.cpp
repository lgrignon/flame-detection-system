//
//  FlameDecider.cpp
//  FlameDetection
//
//  Created by liberize on 14-5-20.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#include "FlameDecider.h"
#include "FlameDetector.h"
#include "FeatureAnalyzer.h"

using cv::ml::TrainData;

const string FlameDecider::SVM_DATA_FILE(string(PROJECT_DIR) + "/svmdata.xml");
// const string FlameDecider::SVM_DATA_FILE("/Users/louisgrignon/Documents/Conscience/flame-detection-system/samples/sample2.txt");

#ifdef TRAIN_MODE
const string FlameDecider::SAMPLE_FILE(string(PROJECT_DIR) + "/samples/sample2.txt");
#endif

#ifdef TRAIN_MODE
FlameDecider::FlameDecider()
: mSampleEnough(false)
, mFlameCount(0)
, mNonFlameCount(0)
, mFrameCount(0)
{
    Feature feature;
    bool isFlame;
    
    ifstream ifs(SAMPLE_FILE);
    while (ifs >> feature >> isFlame) {
        mFeatureVec.push_back(feature);
        mResultVec.push_back(isFlame);
        if (isFlame) {
            mFlameCount++;
        } else {
            mNonFlameCount++;
        }
    }
    ifs.close();
    
    if (mFlameCount >= MIN_SAMPLE_COUNT && mNonFlameCount >= MIN_SAMPLE_COUNT) {
        mSampleEnough = true;
        cout << "Flame count: " << mFlameCount << ", non-flame count: " << mNonFlameCount << "." << endl;
    }
}
#else
FlameDecider::FlameDecider()
{
    mSVM = ml::SVM::load(SVM_DATA_FILE);
    //mSVM->load(SVM_DATA_FILE.c_str());
}
#endif

#ifdef TRAIN_MODE
void FlameDecider::userInput(const map<int, Target>& targets)
{
    ofstream ofs(SAMPLE_FILE, ios::app);
    for (map<int, Target>::const_iterator it = targets.begin(); it != targets.end(); it++) {
        if (it->second.lostTimes > 0) {
            continue;
        }
        
        const Feature& feature = it->second.feature;
        const Rectangle& rect = it->second.region.rect;
        
        Mat temp;
        mFrame.copyTo(temp);
        bool flag = true;
        
        while (true) {
            int key = waitKey(200);
            switch (key) {
                case -1:    // no key pressed
                    rectangle(temp, rect, flag ? Scalar(0, 0, 255) : Scalar(0, 255, 0));
                    namedWindow("temp");
                    moveWindow("temp", 350, 400);
                    imshow("temp", temp);
                    flag = !flag;
                    break;
                case 'y':   // press 'y' to add a positive record to sample
                    ofs << feature << true << endl;
#ifdef DEBUG_OUTPUT
                    cout << "freq: " << feature.frequency << endl;
                    feature.printAreaVec();
#endif
                    mFeatureVec.push_back(feature);
                    mResultVec.push_back(true);
                    mFlameCount++;
                    goto next;
                case 'n':   // press 'n' to add a negative record to sample
                    ofs << feature << false << endl;
                    mFeatureVec.push_back(feature);
                    mResultVec.push_back(false);
                    mNonFlameCount++;
                    goto next;
                case ' ':   // press SPACE to skip current target
                    goto next;
                case 's':   // press 's' to skip current frame
                    goto end;
                case 27:    // press ESC to stop training and exit program
                    trainComplete = true;
                    goto end;
                case 'o':   // press 'o' to stop input and start studying
                    mSampleEnough = true;
                    goto end;
                default:
                    break;
            }
        }

    next:
        if (mFlameCount >= MIN_SAMPLE_COUNT && mNonFlameCount >= MIN_SAMPLE_COUNT) {
            mSampleEnough = true;
            goto end;
        }
    }
    
end:
    ofs.close();
    cout << "Flame count: " << mFlameCount << ", non-flame count: " << mNonFlameCount << "." << endl;
}

void FlameDecider::svmStudy()
{
    assert(mFeatureVec.size() == mResultVec.size());
    
    int size = int(mFeatureVec.size());
	Mat data(size, Feature::LEN, CV_32FC1);
	// Mat label(size, 1, CV_32FC1);
    Mat label(size, 1, CV_32S);
	for (int i = 0; i < size; i++) {
		Mat(mFeatureVec[i]).copyTo(data.row(i));
        label.at<int>(i, 0) = mResultVec[i] ? 1 : 0;
	}

    mSVM->setType(ml::SVM::C_SVC);
    mSVM->setKernel(ml::SVM::LINEAR);
    mSVM->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER|TermCriteria::EPS, 100, 1e-6));

    Ptr<TrainData> td = TrainData::create(data, cv::ml::ROW_SAMPLE, label);
    mSVM->train(td);

    // mSVM.train(data, label, Mat(), Mat(), params);
	mSVM->save(SVM_DATA_FILE.c_str());
}

void FlameDecider::train(const map<int, Target>& targets)
{
    if (!mSampleEnough) {
        if (mFrameCount++ % FRAME_GAP == 0) {
            userInput(targets);
        }
    } else {
        svmStudy();
        trainComplete = true;
    }
}
#else
inline bool FlameDecider::svmPredict(const Feature& feature)
{
    Mat data = Mat(feature);
    Mat data2;
    // .copyTo(data.row(0));
    
    data.convertTo(data2, CV_32F);
        
    float result = mSVM->predict(data2);
    cout << "result: " << result << endl;
	return result == 1.0;
}

bool FlameDecider::judge(map<int, Target>& targets)
{
    bool flameDetected = false;
    
    Mat temp;
    mFrame.copyTo(temp);
    
    for (map<int, Target>::iterator it = targets.begin(); it != targets.end(); it++) {
        bool isFlame = svmPredict(it->second.feature);
        it->second.isFlame = isFlame;
        if (isFlame) {
            flameDetected = true;
            rectangle(temp, it->second.region.rect, Scalar(0, 255, 0));
        }
    }
    
    namedWindow("result");
    moveWindow("result", 350, 400);
    imshow("result", temp);
    return flameDetected;
}
#endif

bool FlameDecider::decide(const Mat& frame, map<int, Target>& targets)
{
    mFrame = frame;
    
#ifdef TRAIN_MODE
    train(targets);
    return false;
#else
    return judge(targets);
#endif
}
