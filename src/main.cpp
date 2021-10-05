#include <iostream>
#include <getopt.h>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <fstream>
#include <numeric>

#include <qcsnpe.hpp>
#include  <utils.hpp>

void print_help() {
    std::cout << "\nDESCRIPTION:\n"
                    << "Application for tracking human through 610\n\n"
                    << "Required Argument:\n"
                    << "-m <Model-Path>: Give path of DLC File. You can avoid this while running to get a reference frame.\n\n"
                    << "Optional Arguments:\n"
                    << "-r <Runtime>: Specify the Runtime to run network Eg, CPU(0), GPU(1), DSP(2), AIP(3)\n"
                    << "-f <Reference-Image-Name>: A reference image will be stored with the given name. \n"
                    << "-c <Count>: Specify the Frame interval to send info to aws (default: 15).\n"
                    << "-i <Image-Path>: Specify the path of the test frame. If not given, takes camera stream.\n";
    return;
}
int main(int argc, char* argv[])
{
    int opt = 0;
    int runtime = 0;  //CPU(0), GPU(1), DSP(2)
    float original_width;
    float original_height;
    float iou;
    int frame_interval = 15;
    std::string image_path;
    std::string model_path;
    std::string reference_image;
    cv::VideoCapture cap;
    cv::VideoWriter video;
    int fourcc = cv::VideoWriter::fourcc('X','V','I','D');
    cv::Mat frame;
    cv::Mat resized_img;
    cv::Rect roi_cordinates;
    std::vector<cv::Rect> found;

    std::vector<std::string> output_layers {OUTPUT_LAYER_1, OUTPUT_LAYER_2};

    while((opt = getopt(argc, argv, "hm:r:i:f:c:")) != -1) {
        switch(opt) {
            case 'h': print_help();
                      return -1;
            case 'm': model_path = optarg;
                      break;
            case 'r': runtime = atoi(optarg);
                      break;
            case 'i': image_path = optarg;
                      break;  
            case 'f': reference_image = optarg;
                      break;    
            case 'c': frame_interval = atoi(optarg);
                      break;                  
            default:  std::cerr << "Invalid parameter specified."<<std::endl;
                      print_help();
                      return -1;
        }
    }

    if(model_path.empty()) {
        if(reference_image.empty()) {
            std::cerr<<"ERROR: Model path or Reference image path not given, exiting the application"<<std::endl;
            print_help();
            return 0;
        } else {
            std::cout<<"Fetching single frame for reference"<<std::endl;
            cap.open(GST_CAM_PIPELINE);
            if (cap.isOpened() == false)  
            {
                std::cout << "Cannot open the video camera" << std::endl;
                std::cin.get();
                return -1;
            }
            original_width = cap.get(cv::CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
            original_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video
            std::cout << "Resolution of the Camera Frame : " << original_width << " x " << original_height << std::endl;        
            if (cap.read(frame) == false) 
            {
                std::cout << "Video camera is disconnected" << std::endl;
                std::cin.get();
                return -1;
            } else {
                std::cout<<"Single frame fetched for reference"<<std::endl;
            }
            cv::imwrite(reference_image, frame);
            std::cout<<"Saved Reference image at: "<<reference_image<<std::endl;

            cap.release();
            return 0;
        }
    }

    Qcsnpe qc(model_path, output_layers, runtime);  //Initialize Qcsnpe Class
    roi_cordinates = parse_json(ROI_CONFIG_PATH); //Parse the config file for cordinates of ROI

    if(!image_path.empty()) {
        unsigned int person = 1;
        unsigned int in_roi_person = 0;

        frame = cv::imread(image_path);
        original_width = frame.cols;
        original_height = frame.rows;
        std::cout << "Resolution of the Frame : " << original_width << " x " << original_height << std::endl;
        video.open("appsrc ! videoconvert ! omxh264enc ! h264parse ! mp4mux ! filesink location=video.mp4", fourcc, 30/1, cv::Size(original_width, original_height), true);
        std::cout<<"----------------------- Frame "<<0<<" -----------------------"<<std::endl;
        cv::resize(frame, resized_img, cv::Size(MOBILENET_IMG_WIDTH, MOBILENET_IMG_HEIGHT)); 

        std::map<std::string, std::vector<float>> out = qc.predict(resized_img);  //Inferenceing
        
        found = postprocess(out, original_height, original_width);  //Postprocessing
        cv::rectangle(frame, roi_cordinates, cv::Scalar(255,0, 0), 2);  //Draw ROI Rectangle
        for(auto r:found) {
            iou = calculate_iou(roi_cordinates, r);
            if(iou < IOU_THRESHOLD) {
                cv::rectangle(frame, r, cv::Scalar(0, 0, 255), 2);
            } else {
                in_roi_person++;
                cv::rectangle(frame, r, cv::Scalar(0, 255, 0), 2);
            }
            cv::putText(frame, "Person " + std::to_string(person), r.tl(), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,255), 1);
            ++person;
        }

        //Time calculation
        auto c_time = std::chrono::system_clock::now();
        std::time_t cur_time = std::chrono::system_clock::to_time_t(c_time);
        char * time_str= std::ctime(&cur_time);
        std::string time_string(time_str);
        time_string.pop_back();

        //Writing on Data on Frame
        cv::putText(frame, time_string, cv::Point(40,560), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255,255,0), 1);
        cv::putText(frame, "Total Count :" +std::to_string(person - 1), cv::Point(40,30), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255,255,0), 1);
        cv::putText(frame, "In ROI Count :" +std::to_string(in_roi_person), cv::Point(40,55), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255,255,0), 1);

        std::cout<<"Total Count :"<<person - 1 << std::endl;
        std::cout<<"People inside ROI :"<<in_roi_person << std::endl;
        std::cout<<"\n\n";
        aws_iot_send(time_str, person, in_roi_person);  //Send info to AWS
        cv::imwrite(OUT_FRAME_PATH, frame);
        std::cout<<"Saved image: "<<OUT_FRAME_PATH<<std::endl;
        return 0;

    } else {
        cap.open(GST_CAM_PIPELINE);
        if (cap.isOpened() == false)  
        {
            std::cout << "Cannot open the video camera" << std::endl;
            std::cin.get();
            return -1;
        }

        original_width = cap.get(cv::CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
        original_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video
        std::cout << "Resolution of the video : " << original_width << " x " << original_height << std::endl;
        video.open(GST_VIDEO_STORE_PIPELINE, fourcc, 30/1, cv::Size(original_width, original_height), true);
        
        for(auto i=0; i<=LOOP_COUNT; i++) {
            unsigned int person = 1;
            unsigned int in_roi_person = 0;

            if (cap.read(frame) == false) 
            {
                std::cout << "Video camera is disconnected" << std::endl;
                std::cin.get();
                break;
            }
            std::cout<<"----------------------- Frame "<<i<<" -----------------------"<<std::endl;

            cv::resize(frame, resized_img, cv::Size(MOBILENET_IMG_WIDTH, MOBILENET_IMG_HEIGHT)); 
            std::map<std::string, std::vector<float>> out = qc.predict(resized_img);
            found = postprocess(out, original_height, original_width);
            cv::rectangle(frame, roi_cordinates, cv::Scalar(255,0, 0), 2);

            for(auto r:found) {
                iou = calculate_iou(roi_cordinates, r);
                if(iou < IOU_THRESHOLD) {
                    cv::rectangle(frame, r, cv::Scalar(0, 0, 255), 2);
                } else {
                    in_roi_person++;
                    cv::rectangle(frame, r, cv::Scalar(0, 255, 0), 2);
                }
                cv::putText(frame, "Person " + std::to_string(person), r.tl(), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,255), 1);
                ++person;
            }

            //Time calculation
            auto c_time = std::chrono::system_clock::now();
            std::time_t cur_time = std::chrono::system_clock::to_time_t(c_time);
            char * time_str= std::ctime(&cur_time);
            std::string time_string(time_str);
            time_string.pop_back();

            //Writing on Data on Frame
            cv::putText(frame, time_string, cv::Point(40,560), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255,255,0), 1);
            cv::putText(frame, "Total Count :" +std::to_string(person - 1), cv::Point(40,30), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255,255,0), 1);
            cv::putText(frame, "In ROI Count :" +std::to_string(in_roi_person), cv::Point(40,55), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255,255,0), 1);
            
            std::cout<<"Total Count :"<<person - 1 << std::endl;
            std::cout<<"People inside ROI :"<<in_roi_person << std::endl;
            std::cout<<"\n\n";

            video.write(frame); //writing output video
            if(i%frame_interval == 0) {
                aws_iot_send(time_str, person, in_roi_person);
            }
        }
    }
    
    std::cout<<"Average throughput: "<<find_average(qc.throughput_vec)<<" ms"<<std::endl;
    std::cout<<"Average FPS: "<<find_average(qc.fps_vec)<<std::endl;
    video.release();
    cap.release();
    return 0;
}
