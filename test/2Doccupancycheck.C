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
#include <filesystem>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;


// Plot2dposition(1, 19, "./testdata/", 400, 400, -4.3, -3.9, 4.3, 4.7)
// Plot2dposition(10, 19, "./testdata/", 1000, 1000, -4.21, -4.11, 4.48, 4.58)

int Plot2dposition(
    int numFiles = 1,                // Number of files to process (1-10)
    int endEventNumber = 19,         // Last event number to process in each file
    const char* baseDir = "./testdata/",  // Directory containing the ROOT files
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
    
    TH2F* h2 = new TH2F("h2", "Photon Distribution", 
                         xBins, xMin, xMax, 
                         yBins, yMin, yMax);
    
    long totalProcessedEvents = 0;
    long totalProcessedPhotons = 0;
    
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
        

        stringstream eventSelectStr;
        if (endEventNumber == 0) {
            eventSelectStr << "Entry$==0";
        } else {
            eventSelectStr << "Entry$>=" << 0 << " && Entry$<=" << endEventNumber;
        }
        string eventSelect = eventSelectStr.str();
        
        stringstream drawStr;
        drawStr << "OP_pos_final_y:OP_pos_final_x";
        string drawCondition = drawStr.str();
        
        string selectionCondition = eventSelect + " && OP_pos_final_z>80 && OP_isCoreC";
        
        TH2F* tempHist = new TH2F("tempHist", "Temp Histogram", 
                                  xBins, xMin, xMax, 
                                  yBins, yMin, yMax);
        
        tree->Draw((drawCondition + ">>tempHist").c_str(), selectionCondition.c_str(), "goff");
        
        double entriesInFile = tempHist->GetEntries();
        totalProcessedPhotons += entriesInFile;
        
        cout << "  Processed " << min(endEventNumber + 1, totalEvents) 
             << " events, found " << entriesInFile << " photons" << endl;
        
        h2->Add(tempHist);
        
        delete tempHist;
        f->Close();
        delete f;
        
        totalProcessedEvents += min(endEventNumber + 1, totalEvents);
    }
    
    if (h2->GetEntries() == 0) {
        cout << "No photons were found with the given conditions across " 
             << numFiles << " files." << endl;
        delete h2;
        return 1;
    }
    
    TCanvas* c = new TCanvas("c1", "Combined Histogram", 800, 600);
    
    stringstream titleStr;
    titleStr << totalProcessedEvents << " events (" 
             << 1000/xBins << "x" << 1000/yBins << " #mu m^{2} )";
    h2->SetTitle(titleStr.str().c_str());
    h2->SetXTitle("x [cm]");
    h2->SetYTitle("y [cm]");
    
    gStyle->SetOptStat(0);
    gStyle->SetStatX(0.1);
    gStyle->SetStatY(0.9);
    
    h2->Draw("colz");
    
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
    cout << "Count: " << maxCount << endl;
    
    double totalEntries = h2->GetEntries();
    cout << "Total entries in combined histogram: " << totalEntries << endl;
    
    stringstream outFileStr;
    outFileStr << "2DHistogram_" << xBins << "x" << yBins 
               << "_" << numFiles << "files_" 
               << totalProcessedEvents << "events.png";
    
    c->SaveAs(outFileStr.str().c_str());
    
    delete c;
    delete h2;
    
    cout << "Summary:" << endl;
    cout << "- Processed " << numFiles << " files" << endl;
    cout << "- Total events: " << totalProcessedEvents << endl;
    cout << "- Total photons: " << totalProcessedPhotons << endl;
    cout << "- Output saved to: " << outFileStr.str() << endl;
    
    return 0;
}
