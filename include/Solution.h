//
// Created by chenghaowang on 30/06/17.
//

#ifndef SOLUTION_H
#define SOLUTION_H

#include "ProcessClock.h"
#include "Solver.h"
#include "Input.h"

#undef IL_STD
#define  IL_STD
#include <ilcplex/ilocplex.h>
std::default_random_engine generator;

qviList Combination(viList viConstraintList, int k) {
    qviList qviCombinationList;
    for (int i = 0; i < (int) viConstraintList.size() - k + 1; i++) {
        viList candidateList;
        candidateList.push_back(i);
        qviCombinationList.push_back(candidateList);
    }
    while ((int) qviCombinationList.front().size() < k) {
        int iValue = qviCombinationList.front()[qviCombinationList.front().size() - 1];
        for (int i = iValue + 1;
             i < (int) viConstraintList.size() - k + 1 + (int) qviCombinationList.front().size(); i++) {
            viList candidateList = viList((viList &&) qviCombinationList.front());
            candidateList.push_back(i);
            qviCombinationList.push_back(candidateList);
        }
        qviCombinationList.pop_front();
    }
    return qviCombinationList;
}

viList getComplement(int iSize, viList candidateList) {
    viList resultList;
    for (int i = 0; i < iSize; i++) {
        auto index = std::find(candidateList.begin(), candidateList.end(), i);
        if (index == candidateList.end()) {
            resultList.push_back(i);
        }
    }
    return resultList;
}

double GaussianDistribution1(const vdList &vdParameter) {
    normal_dist NormalDist((vdParameter[1] - vdParameter[0]) / 2,
                           getSigma1(vdParameter[0], vdParameter[1], vdParameter[2]));
    return NormalDist(generator);
}

double GaussianDistribution2(const vdList &vdParameter) {
    normal_dist NormalDist(0, getSigma2(vdParameter[0], vdParameter[1], vdParameter[2]));
    return NormalDist(generator);
}

double GaussianDistribution3(const vdList &vdParameter) {
    double dSigma = getSigma3(vdParameter[0], vdParameter[1], vdParameter[2], vdParameter[3], vdParameter[4],
                              vdParameter[5]);
    uni_dist UniformDist(0, 1);
    double p = UniformDist(generator);
    if (p < vdParameter[5]) {
        normal_dist NormalDist(vdParameter[0] * vdParameter[3],
                               dSigma);
        return -fabs(NormalDist(generator));
    }else{
        normal_dist NormalDist(vdParameter[1] * vdParameter[4],
                               dSigma);
        return fabs(NormalDist(generator));
    }
}

double **CreateTable(int iSize) {
    double **ppdDouble = new double *[iSize];
    for (int i = 0; i < iSize; i++) {
        ppdDouble[i] = new double[iSize];
    }
    return ppdDouble;
}

void InitTable(double **ppdTable, int iSize) {
    for (int i=0;i<iSize;i++){
        for (int j=0;j<iSize;j++){
            ppdTable[i][j] = 0;
        }
    }
}

void DestroyTable(double **ppdTable, int iSize) {
    for (int i=0;i<iSize;i++){
        delete[] ppdTable[i];
    }
    delete[] ppdTable;
}

void InitConflictedList(const FlightVector &vpCandidateFlightList, FlightVector *vpConflictFlightList, bool bMethod) {
    double dDelay = 0;
    bool bWait = true;
    for (auto &&fi: vpCandidateFlightList) {
        for (auto &&fj: vpCandidateFlightList) {
            if (*fi == *fj) {
                continue;
            }
            double prob = fi->getProbabilityConflictAndDelay(fj, &dDelay, &bWait, bMethod);
            if (prob > 0) {
                vpConflictFlightList->push_back(fi);
                break;
            }
        }
    }
}

void CalculateMi(double **probabilityConflict, double **delayWithoutConflict,
                 int iConflictedFlightSize, vdList *pvdMi) {
    for (int i = 0 ; i < iConflictedFlightSize; i++) {
        double sumPenalCost = 0.0;
        for (int j = 0 ; j < iConflictedFlightSize; j++) {
            if (probabilityConflict[i][j] > 0) {
                sumPenalCost += delayWithoutConflict[i][j];
            }
        }
        pvdMi->push_back(sumPenalCost);
    }
}

void CalculatePi(FlightVector &vpConflictFlightList, vdList *pvdPi) {
    for(auto &&fi: vpConflictFlightList){
        double time = (fi->getArrivingTime() - fi->getDepartureTime());
        pvdPi->push_back(std::max(90.0, time) * fi->getCoefPi());
    }
}

void CalculateConflictProbability(FlightVector &vpConflictFlightList, double **probabilityConflict,
                                  double **delayWithoutConflict, bool bMethod) {
    int iSize = (int) vpConflictFlightList.size();
    bool bWait = true;
    double dDelay = 0;
    for (int i = 0; i < iSize - 1; i++) {
        Flight *fi = vpConflictFlightList[i];
        for (int j = i + 1; j < iSize; j++) {
            Flight *fj = vpConflictFlightList[j];
            probabilityConflict[i][j] = probabilityConflict[j][i] = fi->getProbabilityConflictAndDelay(fj, &dDelay,
                                                                                                       &bWait, bMethod);
            if (bWait) {
                delayWithoutConflict[i][j] = std::max(dDelay, 0.0);
                delayWithoutConflict[j][i] = 0;
            } else {
                delayWithoutConflict[i][j] = 0;
                delayWithoutConflict[j][i] = std::max(dDelay, 0.0);
            }
        }
    }
}

int MinIndexArgs0(viList &viSearchList, int iConflictedFlightSize, double **probabilityConflict,
                  double **delayWithoutConflict, vdList &vdPi, bool bMethod) {
    int iMinIndex = -1;
    double dMaxValue = std::numeric_limits<double>::max();
    if (bMethod) {
        for (auto &&index: viSearchList) {
            double dSumPenalCost = 0;
            for (int j = 0; j < iConflictedFlightSize; j++) {
                dSumPenalCost += delayWithoutConflict[index][j] * probabilityConflict[index][j] / 2;
            }
            //i : min Pi-E[sum{p_ij}]
            dSumPenalCost = vdPi[index] - dSumPenalCost;
            if (dSumPenalCost < dMaxValue) {
                dMaxValue = dSumPenalCost;
                iMinIndex = index;
            }
        }
    }else{
        for (auto &&index: viSearchList) {
            double dSumPenalCost = 0;
            for (int j = 0; j < iConflictedFlightSize; j++) {
                if (probabilityConflict[index][j] > 0) {
                    dSumPenalCost += delayWithoutConflict[index][j];
                }
            }
            //i : min Pi-E[sum{p_ij}]
            dSumPenalCost = vdPi[index] - dSumPenalCost;
            if (dSumPenalCost < dMaxValue) {
                dMaxValue = dSumPenalCost;
                iMinIndex = index;
            }
        }
    }

    if (iMinIndex == -1){
        std::cerr<<"[ERROR] the selected index invalid !" << std::endl;
    }
    return iMinIndex;
}

int MinIndexArgs1(viList &viSearchList, int iConflictedFlightSize, double **probabilityConflict,
                  double **delayWithoutConflict, vdList &vdPi,
                  IloNumArray &decisionVariables, bool bMethod) {
    int iMinIndex = -1;
    double dMaxValue=std::numeric_limits<double>::max();
    if (bMethod) {
        for (auto &&index: viSearchList) {
            double dSumPenalCost = 0;
            for (int j = 0; j < iConflictedFlightSize; j++) {
                dSumPenalCost +=
                        delayWithoutConflict[index][j] * probabilityConflict[index][j] * decisionVariables[j] / 2;
            }
            //i : min Pi-E[sum{p_ij}]
            dSumPenalCost = vdPi[index] - dSumPenalCost;
            if (dSumPenalCost < dMaxValue) {
                dMaxValue = dSumPenalCost;
                iMinIndex = index;
            }
        }
    }else{
        for (auto &&index: viSearchList) {
            double dSumPenalCost = 0;
            for (int j = 0; j < iConflictedFlightSize; j++) {
                if (probabilityConflict[index][j] > 0) {
                    dSumPenalCost += delayWithoutConflict[index][j] * decisionVariables[j];
                }
            }
            //i : min Pi-E[sum{p_ij}]
            dSumPenalCost = vdPi[index] - dSumPenalCost;
            if (dSumPenalCost < dMaxValue) {
                dMaxValue = dSumPenalCost;
                iMinIndex = index;
            }
        }
    }
    if (iMinIndex == -1){
        std::cerr<<"[ERROR] the selected index invalid !" << std::endl;
    }
    return iMinIndex;
}

bool FeasibilityEnumeration(double epsilon, IloNumArray x, vdList Pi, double **delay, double **probability,
                            int iConflictedFlightsSize,
                            int *ind, viList search_list) {
    *ind = -1;
    double dMinFeasibility = std::numeric_limits<double>::max();
    //double min_somme=MAXDOUBLE;
    for (int i = 0; i < iConflictedFlightsSize; i++) {//parcourir les vols dans cette niveau
        if (x[i] == 1) {//traiter seulement les vols qui ont x[i]=1
            int nombre = 0;
            viList candidateList;
            double somme = 0;
            double temp_somme = 0;
            for (int j = 0; j < iConflictedFlightsSize; j++) {
                if (probability[i][j] > 0) {
                    temp_somme += delay[i][j] * x[j];
                    candidateList.push_back(j);//ajouter ce vol dans la liste
                    nombre++;//compter le nombre total des vols qui ont conflict avec vol i
                }
            }
            bool bNotFinish = true;
            int k = nombre;
            //Si la somme est supérieur à Pi
            if (temp_somme > Pi[i]) {
                while (bNotFinish && k > 0) {
                    qviList combinationList = Combination(candidateList, k);
                    bool bValid = false;
                    for (auto element: combinationList) {
//                        std::cout << "combinated List: ";
//                        std::copy(element.begin(), element.end(), std::ostream_iterator<int>(std::cout,"\t"));
//                        std::cout<<std::endl;
                        double temp_prod = 1;
                        double sum_delay = 0.0;
                        viList complement = getComplement((int) candidateList.size(), element);
                        for (auto indexJ: element) {
                            int j = candidateList[indexJ];
                            temp_prod *= probability[i][j];
                            sum_delay += delay[i][j];
                        }
                        for (auto indexJ: complement) {
                            int j = candidateList[indexJ];
                            temp_prod *= 1 - probability[i][j];
                        }
                        if (sum_delay > Pi[i]) {
                            bValid = true;
                            somme += temp_prod;
                        }
                    }
                    k--;
                    bNotFinish = bValid;
                }
                if (somme > epsilon) {
                    return false;
                }

                if (1 - somme < dMinFeasibility && contains(search_list, i)) {
                    dMinFeasibility = somme;
                    *ind = i;
                }
            }
        }//fin boucle x[i]=1
    }//fin boucle parcourir les vols dans cette niveau
    //cout << "i ="<<*ind<<endl;
    return true;
}

bool FeasibilityHoeffding(double dEpsilon, IloNumArray &decisionVariables, vdList &vdPi, double **delayWithoutConflict,
                          int iConflictedFlightSize, double **probabilityConflict, viList search_list, int *piIndex) {
    double dMinFeasibility = std::numeric_limits<double>::max();
    *piIndex = -1;
    for (int i = 0; i <iConflictedFlightSize; i++) {
        double temp = 0;
        double temp2 = 0;
        if (decisionVariables[i] == 1) {
            for (int j = 0; j < iConflictedFlightSize; j++) {
                if (probabilityConflict[i][j] > 0) {
                    temp +=probabilityConflict[i][j] * delayWithoutConflict[i][j] * decisionVariables[j] / 2;
                    temp2 +=pow(delayWithoutConflict[i][j] ,2) * decisionVariables[j];
                }
            }
            temp = exp(-(2 * pow(vdPi[i] - temp, 2)) / temp2);
            if (temp > dEpsilon) {
                return false;
            }
            if (1 - temp < dMinFeasibility && contains(search_list, i)) {
                dMinFeasibility = temp;
                *piIndex = i;
            }
        }
    }
    return true;
}

double getProbaGaussian(double x) {
    return 0.5 * (boost::math::erf(x) + 1);
}

bool FeasibilityGaussian(double dEpsilon, IloNumArray &decisionVariables, vdList &vdPi,
                         FlightVector vpConflictedFlightList, double **ppdProba, double **ppdPenalCost,
                         viList search_list, int *piIndex) {
    int iSize = (int) vpConflictedFlightList.size();
    double dMinFeasibility = std::numeric_limits<double>::max();
    *piIndex = -1;
    for (int i = 0; i < iSize; i++) {
        if (decisionVariables[i] == 1) {
            double exp_mu = 0.0;
            double exp_sigma_2 = 0.0;
            int iCounnter = 0;
            double dLeftProba = 1;
            double mu;
            double sigma_2;
            for (int j = 0; j < iSize; j++) {
                if (ppdProba[i][j] > 0 && decisionVariables[j] == 1) {
                    mu = ppdPenalCost[i][j];
                    sigma_2 = pow(vpConflictedFlightList[i]->getSigma(), 2) +
                              pow(vpConflictedFlightList[j]->getSigma(), 2);
                    dLeftProba *= getProbaGaussian((vdPi[i] - mu) / (sqrt(2 * sigma_2)));
                    exp_mu += mu;
                    exp_sigma_2 += sigma_2;
                    iCounnter++;
                }
            }
            double prob = getProbaGaussian((vdPi[i] - exp_mu) / (sqrt(2 * exp_sigma_2)));
            if (iCounnter > 1) {
                prob *= dLeftProba;
            }
            if (prob < 1 - dEpsilon) {
                return false;
            }
            if (prob < dMinFeasibility && contains(search_list, i)) {
                dMinFeasibility = prob;
                *piIndex = i;
            }
        }
    }
    return true;
}

bool FeasibilityMonteCarlo(double dEpsilon, IloNumArray &decisionVariables, FlightVector &vpConflictedFlightList,
                           int *piIndex, viList &viSearchList, vdList &vdPi, int iModeRandom,
                           vdList &vdParameter) {
    int iConflictedFlightsSize = (int)vpConflictedFlightList.size();
    int iConflictedCounter=0;
    int iMaxConflict = -1;

    double **delayWithoutConflict = CreateTable(iConflictedFlightsSize);
    double **probabilityConflict = CreateTable(iConflictedFlightsSize);
    *piIndex=-1;
    int *table=new int[iConflictedFlightsSize];
    for (int i=0;i<iConflictedFlightsSize;i++){
        table[i]=0;
    }
//    std::cout << "[INFO] Generate a new scenario." << std::endl;
    for (int a=0;a< 1000;a++){
        InitTable(probabilityConflict, iConflictedFlightsSize);
        InitTable(delayWithoutConflict, iConflictedFlightsSize);
        bool test=true;
        // to generate a new scenario from current flight list.
        for (int j = 0; j < iConflictedFlightsSize; j++) {
//            int j = viSearchList[k];
//            if (decisionVariables[j] == 1){
                Flight *fj = vpConflictedFlightList[j];
                Time iOldDepartureTime = fj->getDepartureTime();
                Time iDelta = 0;
                switch (iModeRandom) {
                    case 0:
                        iDelta = GaussianDistribution1(vdParameter);
                        break;
                    case 1:
                        iDelta = GaussianDistribution2(vdParameter);
                        break;
                    default:
                        iDelta = GaussianDistribution3(vdParameter);
                        break;
                }
                fj->GenerateNewFlight(iOldDepartureTime + iDelta, true);
//            }
        }
        CalculateConflictProbability(vpConflictedFlightList, probabilityConflict, delayWithoutConflict, true);
        for (int i = 0; i < iConflictedFlightsSize; i++) {
            if (decisionVariables[i] == 1) {
                double sum = 0;
                for (int j = 0; j < iConflictedFlightsSize; j++) {
                    if (probabilityConflict[i][j] > 0) {
                        sum += decisionVariables[j] * delayWithoutConflict[i][j];
                    }
                }
                if (sum > vdPi[i]) {
                    test = false;
                    table[i] = table[i] + 1;
                    break;
                }
            }
        }
        if (!test){
            iConflictedCounter ++;
        }
        for (auto &&fj: vpConflictedFlightList) {
                fj->resetRouteTimeList();
        }
    }
    for(auto && item : viSearchList){
        if (decisionVariables[item] ==1 && table[item]> iMaxConflict){
            iMaxConflict = table[item];
            *piIndex = item;
        }
    }
    delete table;
    DestroyTable(probabilityConflict, iConflictedFlightsSize);
    DestroyTable(delayWithoutConflict, iConflictedFlightsSize);
//    std::cout << 1000-iConflictedCounter << std::endl;
    if (dEpsilon==0.1){
        if (1000-iConflictedCounter < 915){
            return false;
        }
    }
    if (dEpsilon==0.05){
        if (1000-iConflictedCounter < 961)
        {
            return false;
        }
    }
    if (dEpsilon==0.15){
        if (1000-iConflictedCounter <868){
            return false;
        }
    }
    if (dEpsilon==0.2){
        if (1000-iConflictedCounter < 821){
            return false;
        }
    }
    if (dEpsilon==0.25){
        if (1000-iConflictedCounter <772){
            return false;
        }
    }
    return true;
}

void InitFlightAssignmentMap(const FlightVector &vpFlightList, FlightAssignmentMap *pmpFlightAssignmentMap){
    for(auto &&flight: vpFlightList){
        (*pmpFlightAssignmentMap)[flight] = false;
    }
}

void InitExamineLevel(viList &viLevelsList, LevelExamine *pvpLevelEx) {
    for(auto &&level: viLevelsList){
        (*pvpLevelEx)[level] = false;
    }
}

void InitCandidateFlights(const FlightVector &vpFlightList, FlightAssignmentMap &flightAssignmentMap,
                          LevelExamine &levelEx, int iProcessingLevel, FlightVector *CandidateFlightList){
    for(auto &&flight: vpFlightList){
        const LevelVector &levels = flight->getFeasibleLevelList();
        Level iPreferredLevel = flight->getDefaultLevel();
        if (iPreferredLevel == iProcessingLevel) {
            CandidateFlightList->push_back(flight);
        }
        else {
            for(auto &&level : levels){
                if (level == iProcessingLevel && flightAssignmentMap[flight] == false && levelEx[iPreferredLevel] == true) {
                    CandidateFlightList->push_back(flight);
                    break;
                }
            }
        }
    }
}

void InitAssignedFlights(int iProcessingLevel, FlightVector &CandidateFlightList, FlightVector &ConflictFlightList,
                         double *dSumBenefits,
                         FlightAssignmentMap *flightAssignmentMap) {
    for(auto &&flight: CandidateFlightList){
        // if the flight is not in the actual candidate list.
        if (!contains(ConflictFlightList, flight)) {
            auto index = std::find(flight->getFeasibleLevelList().begin(), flight->getFeasibleLevelList().end(),
                                   iProcessingLevel);
            if (index != flight->getFeasibleLevelList().end()) {
                long lIndex = index - flight->getFeasibleLevelList().begin();
                *dSumBenefits += exp(3 - lIndex);
            }
            flight->setCurrentLevel(iProcessingLevel);
//            if (flight->getDefaultLevel() != iProcessingLevel)
//                std::cout << "[INFO] flight: " << flight->getCode() << " change the level from "
//                          << flight->getDefaultLevel() << " to " << iProcessingLevel << std::endl;
            (*flightAssignmentMap)[flight] = true;
        }
    }
}

void GetMaxConflictCount(double **probability, int iConflictedFlightsSize, int *maxConflictCount) {
    for (int i = 0; i < iConflictedFlightsSize; i++) {
        int temp = 0;
        for (int j = 0; j < iConflictedFlightsSize; j++) {
            if (probability[i][j] > MIN_PROBA) {
                temp++;
            }
        }
        if (temp > *maxConflictCount) {
            *maxConflictCount = temp;
        }
    }
//    std::cout << std::endl << std::endl;
}

FlightVector
SolveFLA(const FlightVector &vpFlightList, LevelVector viLevelsList, IloEnv env, std::ofstream &cplexLogFile,
         double *dSumBenefit, int *iMaxNbConflict, int iModeMethod, double epsilon, vdList vdParameter,
         int iModeRandom) {
    LevelExamine levelEx;
    FlightAssignmentMap flightAssignmentMap;
    InitFlightAssignmentMap(vpFlightList, &flightAssignmentMap);
    InitExamineLevel(viLevelsList, &levelEx);
    *dSumBenefit = 0;
    *iMaxNbConflict = 0;
    for (auto &&fi: vpFlightList) {
        fi->resetLevel();
    }
    for (auto itA = viLevelsList.begin(); itA != viLevelsList.end(); itA++) {
        int iProcessingLevel = (*itA);
//        std::cout << "[INFO] Processing Level: " << iProcessingLevel << std::endl;
        FlightVector CandidateFlightList;
        FlightVector ConflictFlightList;
        vdList Mi, Pi;
//        std::cout << "\tInitialize the Candidate Flight list..." << std::flush;
        InitCandidateFlights(vpFlightList, flightAssignmentMap, levelEx, iProcessingLevel, &CandidateFlightList);
//        std::cout << "OK" << std::endl << "\t\tCandidate flights size: " << CandidateFlightList.size() << std::endl;

//        std::cout << "\tInitialize the Conflicted Flight list..." << std::flush;
        InitConflictedList(CandidateFlightList, &ConflictFlightList, iModeMethod < 3);
//        std::cout << "OK" << std::endl << "\t\tConflicted flights size: " << ConflictFlightList.size() << std::endl;
        levelEx[iProcessingLevel] = true;
//        std::cout << "\tAssign the flights which have no conflict with the other flights in current level..."
//                  << std::endl;
        InitAssignedFlights(iProcessingLevel, CandidateFlightList, ConflictFlightList, dSumBenefit,
                            &flightAssignmentMap);
//        std::cout << "OK" << std::endl << "\t\tNon-Conflicted flights size: "
//                  << CandidateFlightList.size() - ConflictFlightList.size() << std::endl;
        if (ConflictFlightList.size() == 0) {
//            std::cout << "\t[INFO] All flights in current level can be assigned without any conflict." << std::endl;
            continue;
        }
//        std::cout << "\tCreate and Initialize the conflict table..." << std::flush;
        double **probability = CreateTable((int) ConflictFlightList.size());
        double **delai = CreateTable((int) ConflictFlightList.size());
        InitTable(probability, (int) ConflictFlightList.size());
        InitTable(delai, (int) ConflictFlightList.size());
//        std::cout << "OK" << std::endl;
//        std::cout << "\tCalculate the conflict probability and delay time to avoid the conflict..." << std::flush;

        CalculateConflictProbability(ConflictFlightList, probability, delai, iModeMethod < 3);
//        std::cout << "OK" << std::endl;
//        std::cout << "\tCalculate the p_ij and Mi vector..." << std::flush;
        CalculateMi(probability, delai, (int) ConflictFlightList.size(), &Mi);
//        std::cout << "OK" << std::endl;
//        std::cout << "\tCalculate Pi vector..." << std::flush;
        CalculatePi(ConflictFlightList, &Pi);
//        std::cout << "OK" << std::endl;
//        bool bIsAssignedPreferred = false;
//        for (auto &&flight: ConflictFlightList) {
//            if (flight->getDefaultLevel() == iProcessingLevel) {
//                bIsAssignedPreferred = true;
//            }
//        }
//        if (!bIsAssignedPreferred) {
//            std::cout << "\t[INFO] No flight in current will be assigned as its preferred flight level." << std::endl;
//        }
        GetMaxConflictCount(probability, (int) ConflictFlightList.size(), iMaxNbConflict);
        //step one///////////////////////////////////////////////////////////////////////
        // to find the index of aircraft that has minimal sum of Pij.
//        std::cout << "\tGo to step one:" << std::endl;
        viList viSearchList;
        for (int i = 0; i < (int) ConflictFlightList.size(); i++) {
            viSearchList.push_back(i);
        }
        int index;
        index = MinIndexArgs0(viSearchList, (int) ConflictFlightList.size(), probability, delai, Pi,
                              iModeMethod < 3);
//        std::cout << "\t\tMin index:" << index << std::endl;
        viList viConstraintList;
        viConstraintList.push_back(index);
        viSearchList.erase(std::remove(viSearchList.begin(), viSearchList.end(), index), viSearchList.end());
        Solver *solver = new Solver(env, ConflictFlightList, iProcessingLevel, Mi, Pi, delai, viConstraintList,
                                    cplexLogFile, false);
        solver->solve();
        double dFunctionObjectiveValue = solver->getFunctionObjectiveValue();
        IloNumArray decisionVariablesValues = solver->getDecisionVariablesValues();
        delete solver;
        //finish step one////////////////////////////////////////////////////////////////
        int iIndexFromFeasibilityCheck = -1;
        /*Test de feasibility of each constraint*/
        switch (iModeMethod) {
            case 0:
                /*enumeration method*/
//                std::cout << "[INFO] Using Enumeration method" << std::endl;
                while (!FeasibilityEnumeration(epsilon, decisionVariablesValues, Pi, delai, probability,
                                               (int) ConflictFlightList.size(),
                                               &iIndexFromFeasibilityCheck, viConstraintList) &&
                       viSearchList.size() > 0) {
                    //Si on ne trouve pas une bonne index dans la test, on utilise la formule
//                    std::cout << "\tGo to step two:" << std::endl;
                    if (iIndexFromFeasibilityCheck == -1) {
                        index = MinIndexArgs1(viSearchList, (int) ConflictFlightList.size(), probability, delai, Pi,
                                              decisionVariablesValues, true);
                    } else {
                        index = iIndexFromFeasibilityCheck;
                    }
//                    std::cout << "\t\tMin index:" << index << std::endl;
                    viConstraintList.push_back(index);
                    viSearchList.erase(std::remove(viSearchList.begin(), viSearchList.end(), index),
                                       viSearchList.end());
                    solver = new Solver(env, ConflictFlightList, iProcessingLevel, Mi, Pi, delai,
                                        viConstraintList, cplexLogFile, false);
                    solver->solve();
                    dFunctionObjectiveValue = solver->getFunctionObjectiveValue();
                    decisionVariablesValues.end();
                    decisionVariablesValues = solver->getDecisionVariablesValues();
                    delete solver;
                }
                break;
            case 1:
                /*Hoeffding*/
//                std::cout << "[INFO] Using Hoeffding method" << std::endl;
                while (!FeasibilityHoeffding(epsilon, decisionVariablesValues, Pi, delai,
                                             (int) ConflictFlightList.size(), probability, viSearchList,
                                             &iIndexFromFeasibilityCheck) &&
                       viSearchList.size() > 0) {
//                    std::cout << "\tGo to step two:" << std::endl;
                    if (iIndexFromFeasibilityCheck == -1) {
                        index = MinIndexArgs1(viSearchList, (int) ConflictFlightList.size(), probability, delai, Pi,
                                              decisionVariablesValues, true);
                    } else {
                        index = iIndexFromFeasibilityCheck;
                    }
//                    std::cout << "\t\tMin index:" << index << std::endl;
                    viConstraintList.push_back(index);
                    viSearchList.erase(std::remove(viSearchList.begin(), viSearchList.end(), index),
                                       viSearchList.end());
                    solver = new Solver(env, ConflictFlightList, iProcessingLevel, Mi, Pi, delai,
                                        viConstraintList, cplexLogFile, false);

                    solver->solve();
                    dFunctionObjectiveValue = solver->getFunctionObjectiveValue();
                    decisionVariablesValues.end();
                    decisionVariablesValues = solver->getDecisionVariablesValues();
                    delete solver;
                }
                break;
            case 2:/*MonteCarlo*/
//                std::cout << "[INFO] Using Monte-Carlo method" << std::endl;
                while (!FeasibilityMonteCarlo(epsilon, decisionVariablesValues, ConflictFlightList,
                                              &iIndexFromFeasibilityCheck,
                                              viSearchList, Pi, iModeRandom, vdParameter) &&
                       viSearchList.size() > 0) {
                    //Si on ne trouve pas une bonne index dans la test, on utilise la formule
//                    std::cout << "\tGo to step two:" << std::endl;
                    if (iIndexFromFeasibilityCheck == -1) {
                        index = MinIndexArgs1(viSearchList, (int) ConflictFlightList.size(), probability, delai, Pi,
                                              decisionVariablesValues, true);
                    } else {
                        index = iIndexFromFeasibilityCheck;
                    }
//                    std::cout << "\t\tMin index:" << index << std::endl;
                    viConstraintList.push_back(index);
                    viSearchList.erase(std::remove(viSearchList.begin(), viSearchList.end(), index),
                                       viSearchList.end());
                    solver = new Solver(env, ConflictFlightList, iProcessingLevel, Mi, Pi, delai,
                                        viConstraintList, cplexLogFile, false);
                    solver->solve();
                    dFunctionObjectiveValue = solver->getFunctionObjectiveValue();
                    decisionVariablesValues.end();
                    decisionVariablesValues = solver->getDecisionVariablesValues();
                    delete solver;
                }
                break;
            case 3:/*Gaussian*/
//                std::cout << "[INFO] Using Gaussian method" << std::endl;
                while (!FeasibilityGaussian(epsilon, decisionVariablesValues, Pi, ConflictFlightList, probability,
                                            delai, viSearchList, &iIndexFromFeasibilityCheck) &&
                       viSearchList.size() > 0) {
                    //Si on ne trouve pas une bonne index dans la test, on utilise la formule
//                    std::cout << "\tGo to step two:" << std::endl;
                    if (iIndexFromFeasibilityCheck == -1) {
                        index = MinIndexArgs1(viSearchList, (int) ConflictFlightList.size(), probability, delai, Pi,
                                              decisionVariablesValues, false);
                    } else {
                        index = iIndexFromFeasibilityCheck;
                    }
//                    std::cout << "\t\tMin index:" << index << std::endl;
                    viConstraintList.push_back(index);
                    viSearchList.erase(std::remove(viSearchList.begin(), viSearchList.end(), index),
                                       viSearchList.end());
                    solver = new Solver(env, ConflictFlightList, iProcessingLevel, Mi, Pi, delai,
                                        viConstraintList, cplexLogFile, false);
                    solver->solve();
                    dFunctionObjectiveValue = solver->getFunctionObjectiveValue();
                    decisionVariablesValues.end();
                    decisionVariablesValues = solver->getDecisionVariablesValues();
                    delete solver;
                }
                break;

            default:
                std::cerr << "Not support ! " << std::endl;
                abort();
                break;
        }
        *dSumBenefit += dFunctionObjectiveValue;
//        if (viSearchList.size() == 0) {
//            std::cout << "\t[Warning] No solution feasible for this level" << std::endl;
//        }
        for (int i = 0; i < (int) ConflictFlightList.size(); i++) {
            if ((decisionVariablesValues)[i] == 1) {
                Flight *fi = ConflictFlightList[i];
                fi->setCurrentLevel(iProcessingLevel);
//                if (fi->getDefaultLevel() != iProcessingLevel)
//                    std::cout<<"[INFO] "<<fi->getCode()<<" change the level from " << fi->getDefaultLevel() << "to " << iProcessingLevel << std::endl;
                flightAssignmentMap[fi] = true;
            }
        }
        DestroyTable(probability, (int) ConflictFlightList.size());
        DestroyTable(delai, (int) ConflictFlightList.size());
//        std::cout<< "Ok" << std::endl;
    }// end loop of level list
    FlightVector vpFlightNotAssigned;
    for (auto &&fi : vpFlightList) {
        if (flightAssignmentMap[fi] == false) {
            fi->setCoefPi(std::min(fi->getCoefPi() + 0.05, 0.50));
            vpFlightNotAssigned.push_back(fi);
        }
    }
    return vpFlightNotAssigned;
}

int getNbFlightsChangeLevel(const FlightVector &vpFlightList) {
    int iNbFlightChangeLevel = 0;
    for (auto &&fi : vpFlightList) {
        if (fi->getCurrentLevel() != fi->getDefaultLevel()) {
            iNbFlightChangeLevel++;
        }
    }
    return iNbFlightChangeLevel;
}

void ApproximateFLA(Network *pNetwork, double *dSumBenefits, int *iMaxNbConflict, int iModeMethod,
                    double epsilon, int iFeasibleSize, double dCoefPi, vdList vdParameter, int iModeRandom = -1) {
//    int iFeasibleSize=5;
    pNetwork->InitCoefPi(dCoefPi);
    pNetwork->InitFeasibleList(iFeasibleSize);
    pNetwork->InitFlightLevelsList();
    if (iModeRandom > -1) {
        pNetwork->SetSigma(vdParameter, iModeRandom);
    }
    std::ofstream cplexLogFile("cplexLog.txt", std::ios::out | std::ios::app);
    ProcessClock processClock;
    IloEnv env;
    processClock.start();
    const FlightVector &vpFlightList = pNetwork->getFlightsList();
    LevelVector viLevelsList = pNetwork->getFlightLevelsList();
    int iNbFlightNotAssigned = 0;
    int iLastNbFlightNotAssigned = 0;
    int iNbFlightChangeLevel = 0;
//    int iLastNbFlightChangeLevel = 0;
    bool bNotFinish = true;
    std::cout << "[INFO] Starting ApproximateFLA method..." << std::endl;
    try {
        while (bNotFinish) {
            FlightVector vpFlightNotAssigned = SolveFLA(vpFlightList, viLevelsList, env, cplexLogFile, dSumBenefits,
                                                        iMaxNbConflict,
                                                        iModeMethod,
                                                        epsilon, vdParameter, iModeRandom);
            iNbFlightChangeLevel = getNbFlightsChangeLevel(vpFlightList);
            iNbFlightNotAssigned = (int) vpFlightNotAssigned.size();
            std::cout << "[Solved] Nb of flights not be assigned: " << iNbFlightNotAssigned << std::endl;
            std::cout << "[Solved] Nb of flights change level: " << iNbFlightChangeLevel << std::endl << std::endl;
            if (iNbFlightNotAssigned == 0) {
                bNotFinish = false;
            } else if (iNbFlightNotAssigned == iLastNbFlightNotAssigned) {
                for (auto &&fi :vpFlightNotAssigned) {
                    fi->addNewFeasibleLevel(findNextFeasibleLevel(fi->getDefaultLevel(), fi->getLastFeasibleLevel()));
                    Level level = fi->getLastFeasibleLevel();
                    if (!contains(viLevelsList, level)) {
                        viLevelsList.push_back(level);
                    }
                }
//                pNetwork->InitFlightLevelsList();
//                viLevelsList.clear();
//                viLevelsList = pNetwork->getFlightLevelsList();
//                    bNotFinish = false;
            } else {
//                iLastNbFlightChangeLevel = iNbFlightChangeLevel;
                iLastNbFlightNotAssigned = iNbFlightNotAssigned;
            }
        }
    }
    catch (IloException &e) { std::cerr << e.getMessage() << std::endl; }
    catch (...) { std::cerr << "error" << std::endl; }
//    std::cout << "OK" <<std::endl;


    std::cout << "Nb of flights change level: " << iNbFlightChangeLevel << std::endl;
    std::cout << "Nb of flights not be assigned: " << iNbFlightNotAssigned << std::endl;
    std::cout << "Max conflict: " << *iMaxNbConflict << std::endl;
    std::cout << "Sum benefits: " << *dSumBenefits << std::endl;
    cplexLogFile.close();
    env.end();
    processClock.end();
    std::cout << "Elapsed time:" << processClock.getCpuTime() << std::endl;
//    for (auto &&fi :vpFlightList) {
//        std::cout << fi->getFeasibleLevelList().size() << "\n";
//    }
}
#endif //SOLUTION_H
