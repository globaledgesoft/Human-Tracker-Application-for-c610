/**
 * @file utils.cpp
 *
 * @brief Contains implemetation of utlity functions. 
 *
 * @author Arunraj A P
 *
 */

#include  <utils.hpp>

/************************************************************************
* Name : parse_json
* Function: Parse the json file for cordinates of Region of interest
* Returns: OpenCV Rect
************************************************************************/
cv::Rect parse_json(std::string filepath) {
    std::string line;
    int cord;
    std::vector<int> cord_vec;
    std::string temp;
    std::ifstream fin;
    fin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        fin.open(filepath);
    } catch (std::exception &e) {
        std::cerr << e.what() << "\nCould not load config.json\nExiting application"<<std::endl;
        exit(0);
    }
    getline(fin, line);
    std::stringstream ss(line);
    while(ss.good()) {
        ss >> temp;
        if(std::stringstream(temp) >> cord) {
            cord_vec.push_back(cord);
        }
    }
    cv::Rect ref_rect(cv::Point(cord_vec[0], cord_vec[1]), cv::Point(cord_vec[2], cord_vec[3]));
    
    return ref_rect;
} 

/************************************************************************
* Name : calculate_iou
* Function: Caluculate iou for 2 rectangles
* Returns: Float iou value
************************************************************************/
float calculate_iou(cv::Rect rec1, cv::Rect rec2) {
    int xA, yA, xB, yB, interArea, boxAArea, boxBArea;
    float iou;

    xA = std::max(rec1.x, rec2.x);
    yA = std::max(rec1.y, rec2.y);
    xB = std::min(rec1.br().x, rec2.br().x);
    yB = std::min(rec1.br().y, rec2.br().y);
    interArea = std::max(0, xB - xA + 1) * std::max(0, yB - yA + 1);
    boxAArea = (rec1.br().x - rec1.x + 1) * (rec1.br().y - rec1.y + 1);
    boxBArea = (rec2.br().x - rec2.x + 1) * (rec2.br().y - rec2.y + 1);
    iou = interArea / float(boxAArea + boxBArea - interArea);

    return iou;
}

/************************************************************************
* Name : find_average
* Function: Find average of all elements in vector
* Returns: Average value in float
************************************************************************/
float find_average(std::vector<float> &vec) {
    return 1.0 * (std::accumulate(vec.begin(), vec.end(), 0.0) / vec.size());
}

/************************************************************************
* Name : postprocess
* Function: Postprocess the outmap from QCSNPE predict
* Returns: Vector containing coordinates of boxes
************************************************************************/
std::vector<cv::Rect> postprocess(std::map<std::string, std::vector<float>> out, float video_height, float video_width) {
    float probability;
    int class_index;
    std::vector<cv::Rect> found;

    auto &boxes = out[BOXES_TENSOR];
    auto &scores = out[SCORES_TENSOR];
    auto &classes = out[CLASSES_TENSOR];

    for(size_t cur=0; cur < scores.size(); cur++) {
        probability = scores[cur];
        class_index = static_cast<int>(classes[cur]);

        if(class_index != PERSON_CLASS_INDEX || probability < PROBABILITY_THRESHOLD)
            continue;
        
        auto y1 = static_cast<int>(boxes[4 * cur] * video_height);
        auto x1 = static_cast<int>(boxes[4 * cur + 1] * video_width);
        auto y2 = static_cast<int>(boxes[4 * cur + 2] * video_height);
        auto x2 = static_cast<int>(boxes[4 * cur + 3] * video_width);
        found.push_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
    }

    return found;
}

/************************************************************************
* Name : aws_iot_send
* Function: Send MQTT message by invoking python script
* Returns: True if success
************************************************************************/
bool aws_iot_send(char *time_str, unsigned int total_people_count, unsigned int inframe_count) {
    pid_t pid;
    int status1;

    std::cout<<"Sending inferenced data to AWS...."<<std::endl;
    if((pid = fork()) < 0) {
        std::cerr<<"Error while forking boto3"<<std::endl;
        exit(0);
    } else if(pid == 0) {
        char *progName = "python3";
        std::string ppl_count_str = std::to_string(total_people_count - 1);
        char *ppl_count = const_cast<char*> (ppl_count_str.c_str());
        std::string inframe_ppl_count_str = std::to_string(inframe_count);
        char *inframe_ppl_count = const_cast<char*> (inframe_ppl_count_str.c_str());
        char *args[] = {progName, AWS_SCRIPT_PATH, time_str, ppl_count, inframe_ppl_count, NULL};                
        execvp(progName, args);
    }
    waitpid(pid, &status1, 0);

    return true;
}