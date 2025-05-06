#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TDirectory.h"
#include "TROOT.h"
#include "TStyle.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <iomanip>

using namespace std;
namespace fs = std::filesystem;

// root -l -q 'deadtimecheck.C(0.0)'
// Plot2dposition(10, 19, "./testdata/", 10.0, SAVE_PNG, 40, 40, -4.21, -4.11, 4.48, 4.58)
// Plot2dposition(10, 19, "./testdata/", 0.0, SAVE_ROOT, 80, 80, -4.21, -4.11, 4.48, 4.58)


enum SaveMode {
    SAVE_PNG = 0, 
    SAVE_ROOT = 1
};

bool isInsideCircle(double x, double y, double centerX, double centerY, double radius) {
    double dx = x - centerX;
    double dy = y - centerY;
    return (dx*dx + dy*dy) <= radius*radius;
}



int Plot2dposition(
    int numFiles = 1,                // Number of files to process (1-10)
    int endEventNumber = 19,         // Last event number to process in each file
    const char* baseDir = "./testdata/",  // Directory containing the ROOT files
    double deadTime = 0.0,           // Dead time in ns
    int saveMode = SAVE_PNG,         // 0=PNG, 1=ROOT
    int xBins = 400,
    int yBins = 400,
    double xMin = -4.3,
    double xMax = -3.9,
    double yMin = 4.3,
    double yMax = 4.7
) {
    if (numFiles <= 0 || numFiles > 10) {
        cout << "Number of files must be between 1 and 10" << endl;
        return 1;
    }
    
    if (endEventNumber < 0 || endEventNumber >= 20) {
        cout << "Event number must be between 0 and 19" << endl;
        return 1;
    }
    
    if (saveMode != SAVE_PNG && saveMode != SAVE_ROOT) {
        cout << "Invalid save mode. Use 0 for PNG or 1 for ROOT" << endl;
        return 1;
    }
    
    TH2F* h2 = new TH2F("h2", "Photon Distribution", 
                         xBins, xMin, xMax, 
                         yBins, yMin, yMax);
    
    long totalProcessedEvents = 0;
    long totalProcessedPhotons = 0;
    long totalRejectedByDeadtime = 0;
    long totalAddedPhotons = 0;

    double centerX = -4.16; 
    double centerY = 4.527;  
    double radius = 0.04;  
    
    for (int fileNum = 1; fileNum <= numFiles; fileNum++) {
        stringstream fileNameStr;
        fileNameStr << baseDir << "mc_e+_job_rod43_layer42_run43_" << fileNum 
                    << "_Test_20evt_e+_100_100.root";
        string fileName = fileNameStr.str();
        
        if (!fs::exists(fileName)) {
            cout << "Warning: File does not exist: " << fileName << endl;
            continue;
        }
        
        cout << "Processing file " << fileNum << "/" << numFiles << ": " << fileName << endl;
        
        TFile* f = TFile::Open(fileName.c_str());
        if (!f || f->IsZombie()) {
            cout << "Failed to open file: " << fileName << endl;
            continue;
        }
        
        TTree* tree = (TTree*)f->Get("tree;4");
        if (!tree) {
            cout << "Failed to get tree 'tree;4' from file: " << fileName << endl;
            f->Close();
            continue;
        }
        
        int totalEvents = tree->GetEntries();
        cout << "  File contains " << totalEvents << " events" << endl;
        
        vector<double> *pos_x = 0;
        vector<double> *pos_y = 0;
        vector<double> *pos_z = 0;
        vector<double> *time_final = 0;
        vector<bool> *isCoreC = 0;
        
        tree->SetBranchAddress("OP_pos_final_x", &pos_x);
        tree->SetBranchAddress("OP_pos_final_y", &pos_y);
        tree->SetBranchAddress("OP_pos_final_z", &pos_z);
        tree->SetBranchAddress("OP_time_final", &time_final);
        tree->SetBranchAddress("OP_isCoreC", &isCoreC);
        
        int processedEventsInFile = 0;
        long photonsInFile = 0;
        long rejectedByDeadtimeInFile = 0;
        long addedPhotonsInFile = 0;
        
        for (int event = 0; event <= min(endEventNumber, totalEvents-1); event++) {
            tree->GetEntry(event);
            
            TH2F* eventHist = new TH2F(Form("eventHist_%d", event), "Event Histogram", 
                                      xBins, xMin, xMax, 
                                      yBins, yMin, yMax);
            
            map<int, double> lastTimeInBin;
            
            int numPhotons = pos_x->size();
            for (int i = 0; i < numPhotons; i++) {
                if ((*pos_z)[i] <= 80 || (*isCoreC)[i]==0) continue;
                if ( (*pos_x)[i] <= xMin || (*pos_x)[i] >= xMax ) continue;
                if ( (*pos_y)[i] <= yMin || (*pos_y)[i] >= yMax ) continue;
                if (!isInsideCircle((*pos_x)[i], (*pos_y)[i], centerX, centerY, radius)) continue;
 
                int binX = eventHist->GetXaxis()->FindBin((*pos_x)[i]);
                int binY = eventHist->GetYaxis()->FindBin((*pos_y)[i]);
                int binKey = binX * 1000000 + binY;
                    
                double currentTime = (*time_final)[i];
                    
                bool canAdd = true;

                if (lastTimeInBin.find(binKey) != lastTimeInBin.end()) {
                    double lastTime = lastTimeInBin[binKey];
                    if (std::abs(currentTime - lastTime) < deadTime) {
                        canAdd = false;
                        rejectedByDeadtimeInFile++;
                    }
                }
                    
                if (canAdd) {
                    eventHist->SetBinContent(binX, binY, eventHist->GetBinContent(binX, binY) + 1);
                    lastTimeInBin[binKey] = currentTime;
                    photonsInFile++;
                }
                
            }

            for (int ix = 1; ix <= xBins; ix++) {
                for (int iy = 1; iy <= yBins; iy++) {
                    if (eventHist->GetBinContent(ix, iy) > 0) {
                        double binContent = eventHist->GetBinContent(ix, iy);
                        h2->AddBinContent(h2->GetBin(ix, iy), binContent);
                        addedPhotonsInFile += binContent;
                    }
                }
            }
            
            delete eventHist;
            processedEventsInFile++;
        }
        
        totalProcessedPhotons += photonsInFile;
        totalAddedPhotons += addedPhotonsInFile;
        totalProcessedEvents += processedEventsInFile;
        totalRejectedByDeadtime += rejectedByDeadtimeInFile;
        
        cout << "  Processed " << processedEventsInFile << " events" << endl;
        cout << "  Found " << photonsInFile << " photons passing criteria" << endl;
        cout << "  Added " << addedPhotonsInFile << " photons to histogram" << endl;
        cout << "  Rejected " << rejectedByDeadtimeInFile << " photons due to " << deadTime << " ns deadtime" << endl;
        
        f->Close();
        delete f;
    }
    
    if (h2->GetEntries() == 0 && totalAddedPhotons == 0) {
        cout << "No photons were found with the given conditions across " 
             << numFiles << " files." << endl;
        delete h2;
        return 1;
    }
    
    h2->SetEntries(totalAddedPhotons);
    
    stringstream fileBaseStr;
    if (deadTime > 0) {
        fileBaseStr << "2DHistogram_Deadtime" << fixed << setprecision(1) << deadTime << "ns_" 
                   << xBins << "x" << yBins 
                   << "_" << numFiles << "files_" 
                   << totalProcessedEvents << "events";
    } else {
        fileBaseStr << "2DHistogram_NoDeadtime_" 
                   << xBins << "x" << yBins 
                   << "_" << numFiles << "files_" 
                   << totalProcessedEvents << "events";
    }
    string fileBase = fileBaseStr.str();
    
    stringstream titleStr;
    if (deadTime > 0) {
        titleStr << totalProcessedEvents << " events, " << deadTime << " ns deadtime ("
                 << 1000/xBins << "x" << 1000/yBins << " #mu m^{2})";
    } else {
        titleStr << totalProcessedEvents << " events ("
                 << 1000/xBins << "x" << 1000/yBins << " #mu m^{2})";
    }
    h2->SetTitle(titleStr.str().c_str());
    h2->SetXTitle("x [cm]");
    h2->SetYTitle("y [cm]");
    
    string outputFileName;
    
    if (saveMode == SAVE_PNG) {

        TCanvas* c = new TCanvas("c1", "Combined Histogram", 800, 600);
        
        gStyle->SetOptStat(0);
        gStyle->SetStatX(0.1);
        gStyle->SetStatY(0.9);
        
        h2->Draw("colz");
        
        outputFileName = fileBase + ".png";
        c->SaveAs(outputFileName.c_str());
        
        delete c;
    } else {

        outputFileName = fileBase + ".root";
        TFile* outFile = new TFile(outputFileName.c_str(), "RECREATE");
        
        TNamed* infoNumFiles = new TNamed("NumFiles", to_string(numFiles).c_str());
        TNamed* infoNumEvents = new TNamed("NumEvents", to_string(totalProcessedEvents).c_str());
        TNamed* infoDeadTime = new TNamed("DeadTime", to_string(deadTime).c_str());
        TNamed* infoRejected = new TNamed("RejectedPhotons", to_string(totalRejectedByDeadtime).c_str());
        
        infoNumFiles->Write();
        infoNumEvents->Write();
        infoDeadTime->Write();
        infoRejected->Write();
        
        h2->Write("photonDistribution");
        
        outFile->Close();
        delete outFile;
        delete infoNumFiles;
        delete infoNumEvents;
        delete infoDeadTime;
        delete infoRejected;
    }
    
    int maxBin = h2->GetMaximumBin();
    int binX, binY, binZ;
    h2->GetBinXYZ(maxBin, binX, binY, binZ);
    
    double maxCount = h2->GetBinContent(maxBin);
    
    double binWidth_x = (xMax - xMin) / xBins;
    double binWidth_y = (yMax - yMin) / yBins;
    
    double xLow = xMin + (binX-1) * binWidth_x;
    double xHigh = xMin + binX * binWidth_x;
    double yLow = yMin + (binY-1) * binWidth_y;
    double yHigh = yMin + binY * binWidth_y;
    
    double xCenter = (xLow + xHigh) / 2;
    double yCenter = (yLow + yHigh) / 2;
    
    cout << "\nInformation of the maximum bin:" << endl;
    cout << "Bin number: (" << binX << ", " << binY << ")" << endl;
    cout << "Bin position: x = [" << xLow << ", " << xHigh << "], center = " << xCenter << endl;
    cout << "Bin position: y = [" << yLow << ", " << yHigh << "], center = " << yCenter << endl;
    cout << "Count (photons in this bin): " << maxCount << endl;
    
    delete h2;
    
    cout << "\nSummary:" << endl;
    cout << "- Processed " << numFiles << " files" << endl;
    cout << "- Total events: " << totalProcessedEvents << endl;
    cout << "- Total photons passing criteria: " << totalProcessedPhotons << endl;
    cout << "- Total photons added to histogram: " << totalAddedPhotons << endl;
    if (deadTime > 0) {
        cout << "- Total photons rejected by " << deadTime << " ns deadtime: " 
             << totalRejectedByDeadtime << endl;
        
        if (totalProcessedPhotons + totalRejectedByDeadtime > 0) {
            double rejectionPercentage = 100.0 * totalRejectedByDeadtime / 
                                        (totalProcessedPhotons + totalRejectedByDeadtime);
            cout << "- Rejection rate: " << fixed << setprecision(2) 
                 << rejectionPercentage << "%" << endl;
        }
    }
    cout << "- Output saved to: " << outputFileName << endl;
    
    return 0;
}

void deadtimecheck(double deadTime = 0.0) {
    cout << "Running with deadTime = " << deadTime << " ns" << endl;
    
    vector<pair<int, int>> binConfigurations = {
        {20, 20},
        {25, 25},
        {50, 50},
        {100, 100}
    };
    
    for (const auto& binConfig : binConfigurations) {
        int xBins = binConfig.first;
        int yBins = binConfig.second;
        
        cout << "\n\n==============================================" << endl;
        cout << "Running with " << xBins << "x" << yBins << " bins" << endl;
        cout << "==============================================" << endl;
        
        Plot2dposition(10, 19, "./testdata/", deadTime, SAVE_ROOT, 
                      xBins, yBins, -4.21, -4.11, 4.48, 4.58);
    }
    
    cout << "\nAll configurations completed successfully!" << endl;
}
