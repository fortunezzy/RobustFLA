#include "../include/Solution.h"

double Network::PERIOD_LENGTH = 30;

void EchoUsage(){
    std::cout << "[USAGE] RobustFLA airport_json_File beacon_json_file flight_json_file method_mode period_length epsilon coefficient_Pi [random_mode alpha beta gamma [w1 w2 p]]"<< std::endl
            << "method_mode 0 \t\t\t Enumeration Method, the parameters: method_mode period_length epsilon coefficient_Pi required. "<< std::endl
            << "method_mode 1 \t\t\t Hoeffding Inequalities Method, the parameters: method_mode period_length epsilon coefficient_Pi required. "<< std::endl
            << "method_mode 2 \t\t\t Monte-Carlo Simulation Method, the parameters: method_mode period_length epsilon coefficient_Pi random_mode alpha beta gamma required. if random_mode is 2, then the parameters: w1 w2 p required."<< std::endl
            << "method_mode 3 \t\t\t Gaussian Method, the parameters: method_mode period_length epsilon coefficient_Pi random_mode alpha beta gamma required. if random_mode is 2, then the parameters: w1 w2 p required."<< std::endl
            << "period_length \t\t\t An integer, the length of period that consider en-route conflict occurs at a waypoint."<< std::endl
            << "epsilon \t\t\t An integer in [0, 100],  the tolerance of infeasibility of separated constraints in FLA problem."<< std::endl
            << "coefficient_Pi \t\t\t An integer in [0, 100], the percentage of aircraft's flight time to induce an admissible penal cost"<< std::endl
            << "random_mode 0 \t\t\t the generated departure time follows a normal distribution, where the mean is t+(beta-alpha)/2, and that the generated departure time has a confident of gamma at an interval [t-alpha, t+beta]."<< std::endl
            << "random_mode 1 \t\t\t the generated departure time follows a normal distribution, where the mean is t, and that the generated departure time has a confident of gamma at an interval [t-alpha, t+beta]."<< std::endl
            <<"random_mode 2 \t\t\t the generated departure time follows a normal distribution, where the mean is t-w1*alpha in left part Pr(x<t)=p and is t+w12*beta in right part Pr(x>=t)=1-p, and that the generated departure time has a confident of gamma at an interval [t-alpha, t+beta]."<< std::endl
            << "alpha \t\t\t An integer, the lower bound for the interval [t-alpha, t+beta]" << std::endl
                                                                                             << "beta \t\t\t An integer, the upper bound for the interval [t-alpha, t+beta]" << std::endl
                                                                                             << "gamma \t\t\t An integer in [0, 100], Pr(t-alpha<=x<=t+beta)>=gamma%, the confident that generated departure time fall in an interval [t-alpha, t+beta]" << std::endl
                                                                                             << "w1 \t\t\t An integer in [0, 100], the mean u=t-w1*alpha of the left part x<t" << std::endl
                                                                                             << "w2 \t\t\t An integer in [0, 100], the mean u=t+w2*beta of the right part x>=t" << std::endl
                                                                                             << "p \t\t\t An integer in [0, 100], the probability that Pr(x<t)=p%"
                                                                                             << std::endl;
}
int main(int argc, char *argv[]) {
    using std::invalid_argument;
    try {
        if (argc < 9) {
            EchoUsage();
            abort();
        }
        Network::PERIOD_LENGTH = boost::lexical_cast<double>(argv[5]);
        Network network;
        Input input(argv[1], argv[2], argv[3]);
        int iMethodMode = boost::lexical_cast<int>(argv[4]);
        double dSumBenefits = 0;
        int iMaxNbConflict = 0;
        vdList vdParameters;
        int iRandomMode;
        switch (iMethodMode){
            case 0:
            case 1:
                input.initNetwork(&network);
                ApproximateFLA(&network, &dSumBenefits, &iMaxNbConflict, boost::lexical_cast<int>(argv[4]),
                               boost::lexical_cast<double>(argv[6]) / 100.0, boost::lexical_cast<int>(argv[7]),
                               boost::lexical_cast<double>(argv[8]) / 100.0, vdParameters);
                break;
            case 2:
            case 3:
                iRandomMode = boost::lexical_cast<int>(argv[9]);
                if (iRandomMode > -1) {
                    vdParameters.push_back(boost::lexical_cast<double>(argv[10]));
                    vdParameters.push_back(boost::lexical_cast<double>(argv[11]));
                    vdParameters.push_back(boost::lexical_cast<double>(argv[12]) / 100.0);
                }
                if (iRandomMode > 1) {
                    vdParameters.push_back(boost::lexical_cast<double>(argv[13]) / 100.0);
                    vdParameters.push_back(boost::lexical_cast<double>(argv[14]) / 100.0);
                    vdParameters.push_back(boost::lexical_cast<double>(argv[15]) / 100.0);
                }
//                std::copy(vdParameters.begin(), vdParameters.end(), std::ostream_iterator<double>(std::cout, "\t"));
                input.initNetwork(&network);
                ApproximateFLA(&network, &dSumBenefits, &iMaxNbConflict, boost::lexical_cast<int>(argv[4]),
                               boost::lexical_cast<double>(argv[6]) / 100.0, boost::lexical_cast<int>(argv[7]),
                               boost::lexical_cast<double>(argv[8]) / 100.0, vdParameters, iRandomMode);
                break;
            default:
                std::cerr<<"[ERROR] Don't support this method" <<std::endl;
                break;
        }
    }
    catch (const invalid_argument &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}