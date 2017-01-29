#include "IMU.h"
#include "math.h"

inline double to_degrees(double radians) {
    return radians * (180.0 / M_PI);
}


void IMU::setup(){
    RTIMUSettings *settings = new RTIMUSettings("RTIMULib");
    
    imu = RTIMU::createIMU(settings);
    
    if ((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL)) {
        printf("No IMU found\n");
        exit(1);
    }
    
    //  This is an opportunity to manually override any settings before the call IMUInit
    imu->IMUInit();
    
    //  this is a convenient place to change fusion parameters
    
    imu->setSlerpPower(0.02);
    imu->setGyroEnable(true);
    imu->setAccelEnable(true);
    imu->setCompassEnable(true);
    roll.setup(100);
    pitch.setup(100);
    yaw.setup(40);
    mX.setup(70);
    mY.setup(70);
    rollRate.setup(20);
    pitchRate.setup(20);
}

int IMU::updateIMU(){
    int sampleCount = 0;
    int sampleRate = 0;
    if (imu->IMURead()) {
        RTIMU_DATA imuData = imu->getIMUData();
        while (imu->IMURead()) {
            imuData = imu->getIMUData();
        }
        sampleCount++;
        roll.insert(to_degrees(imuData.fusionPose.x()) + rollComp);
        pitch.insert(to_degrees(imuData.fusionPose.y()) + pitchComp);
        yaw.insert(to_degrees(imuData.fusionPose.z()));
        
        mX.insert(imuData.accel.x()+  mXComp);
        mY.insert(imuData.accel.y() + mYComp);
        rollRate.insert(imuData.gyro.x());
        pitchRate.insert(imuData.gyro.y());
        //rotation(1,imuData.fusionPose.x(),imuData.fusionPose.y(),imuData.fusionPose.z());
        //printf("Test one piece: Roll = %f\n",to_degrees(imuData.fusionPose.data(0)));
        //printf("Sample rate %d: %s\r", sampleRate, RTMath::displayDegrees("", imuData.fusionPose));
        return 1;
    }else{
        return 0;
    }

}

void IMU::resetIMUFusion(){
    imu->resetFusion();
}








