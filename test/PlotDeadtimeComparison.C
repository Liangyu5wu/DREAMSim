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
#include <algorithm>
#include "TString.h"

using namespace std;

int PlotDeadtimeComparison(
    int pixelSize = 100,            // Pixel size (20, 25, 40, 50, 80, 100)
    double deadtime = 30.0,         // Deadtime value in ns (0.0 for NoDeadtime, or 1.0, 2.0, 5.0, 10.0, 30.0)
    const char* baseDir = "../deadtimedata/"  // Directory containing the ROOT files
) {
    // Validate pixel size
    if (pixelSize != 20 && pixelSize != 25 && pixelSize != 40 && 
        pixelSize != 50 && pixelSize != 80 && pixelSize != 100) {
        cout << "Pixel size must be one of: 20, 25, 40, 50, 80, 100" << endl;
        return 1;
    }
    
    // Calculate actual pixel size in μm
    int actualPixelSize = 1000 / pixelSize;
    
    // Create file names
    TString noDeadtimeFileName;
    TString deadtimeFileName;
    
    noDeadtimeFileName.Form("%s2DHistogram_NoDeadtime_%dx%d_10files_200events.root", 
                           baseDir, pixelSize, pixelSize);
    
    if (deadtime <= 0.0) {
        deadtimeFileName = noDeadtimeFileName;
    } else {
        deadtimeFileName.Form("%s2DHistogram_Deadtime%.1fns_%dx%d_10files_200events.root", 
                             baseDir, deadtime, pixelSize, pixelSize);
    }
    
    cout << "Processing files:" << endl;
    cout << "- No deadtime: " << noDeadtimeFileName << endl;
    cout << "- With deadtime: " << deadtimeFileName << endl;
    
    // Open files
    TFile* noDeadtimeFile = TFile::Open(noDeadtimeFileName.Data());
    if (!noDeadtimeFile || noDeadtimeFile->IsZombie()) {
        cout << "Failed to open file: " << noDeadtimeFileName << endl;
        return 1;
    }
    
    TFile* deadtimeFile = TFile::Open(deadtimeFileName.Data());
    if (!deadtimeFile || deadtimeFile->IsZombie()) {
        cout << "Failed to open file: " << deadtimeFileName << endl;
        noDeadtimeFile->Close();
        return 1;
    }
    
    // Get histograms
    TH2F* noDeadtimeHist = (TH2F*)noDeadtimeFile->Get("photonDistribution");
    if (!noDeadtimeHist) {
        cout << "Failed to get 'photonDistribution' histogram from: " << noDeadtimeFileName << endl;
        noDeadtimeFile->Close();
        deadtimeFile->Close();
        return 1;
    }
    
    TH2F* deadtimeHist = (TH2F*)deadtimeFile->Get("photonDistribution");
    if (!deadtimeHist) {
        cout << "Failed to get 'photonDistribution' histogram from: " << deadtimeFileName << endl;
        noDeadtimeFile->Close();
        deadtimeFile->Close();
        return 1;
    }
    
    // Clone histograms to avoid modifying the originals
    TH2F* noDeadtimeHistClone = (TH2F*)noDeadtimeHist->Clone("noDeadtimeHistClone");
    TH2F* deadtimeHistClone = (TH2F*)deadtimeHist->Clone("deadtimeHistClone");
    
    // Create the ratio histogram
    TH2F* ratioHist = (TH2F*)noDeadtimeHist->Clone("ratioHist");
    ratioHist->Reset();
    
    // Calculate ratio
    int xBins = noDeadtimeHist->GetNbinsX();
    int yBins = noDeadtimeHist->GetNbinsY();
    
    for (int i = 1; i <= xBins; i++) {
        for (int j = 1; j <= yBins; j++) {
            double noDeadtimeVal = noDeadtimeHist->GetBinContent(i, j);
            double deadtimeVal = deadtimeHist->GetBinContent(i, j);
            
            if (noDeadtimeVal > 0) {
                double ratio = deadtimeVal / noDeadtimeVal;
                ratioHist->SetBinContent(i, j, ratio);
            } else {
                ratioHist->SetBinContent(i, j, 0);
            }
        }
    }
    
    // Set up style - similar to the original style
    gStyle->SetOptStat(0);
    gStyle->SetStatX(0.1);
    gStyle->SetStatY(0.9);
    
    // Title for histograms - fixed format specifiers
    TString noDeadtimeTitle = Form("No Deadtime - %dx%d #mu m^{2}", actualPixelSize, actualPixelSize);
    TString deadtimeTitle = Form("Deadtime %.1f ns - %dx%d #mu m^{2}", deadtime, actualPixelSize, actualPixelSize);
    TString ratioTitle = Form("Reception Ratio - %dx%d #mu m^{2}", actualPixelSize, actualPixelSize);
    
    noDeadtimeHistClone->SetTitle(noDeadtimeTitle.Data());
    deadtimeHistClone->SetTitle(deadtimeTitle.Data());
    ratioHist->SetTitle(ratioTitle.Data());
    
    // Set axes titles
    noDeadtimeHistClone->SetXTitle("x [cm]");
    noDeadtimeHistClone->SetYTitle("y [cm]");
    
    deadtimeHistClone->SetXTitle("x [cm]");
    deadtimeHistClone->SetYTitle("y [cm]");
    
    ratioHist->SetXTitle("x [cm]");
    ratioHist->SetYTitle("y [cm]");
    
    // Draw individual canvases
    TCanvas* c1 = new TCanvas("c1", "No Deadtime", 800, 700);
    noDeadtimeHistClone->Draw("colz");
    
    TString noDeadtimeOutFileName = Form("NoDeadtime_%dx%d.png", pixelSize, pixelSize);
    c1->SaveAs(noDeadtimeOutFileName.Data());
    
    TCanvas* c2 = new TCanvas("c2", "With Deadtime", 800, 700);
    deadtimeHistClone->Draw("colz");
    
    TString deadtimeOutFileName = Form("Deadtime%.1fns_%dx%d.png", deadtime, pixelSize, pixelSize);
    c2->SaveAs(deadtimeOutFileName.Data());
    
    TCanvas* c3 = new TCanvas("c3", "Ratio", 800, 700);
    ratioHist->Draw("colz");
    
    TString ratioOutFileName = Form("ReceptionRatio_Deadtime%.1fns_%dx%d.png", deadtime, pixelSize, pixelSize);
    c3->SaveAs(ratioOutFileName.Data());
    
    // Calculate rejection rate for output
    double totalEntries_noDeadtime = noDeadtimeHistClone->GetEntries();
    double totalEntries_deadtime = deadtimeHistClone->GetEntries();
    
    cout << "\nTotal entries in No Deadtime histogram: " << totalEntries_noDeadtime << endl;
    cout << "Total entries in Deadtime histogram: " << totalEntries_deadtime << endl;
    cout << "Rejection rate: " << 100.0 * (1.0 - totalEntries_deadtime / totalEntries_noDeadtime) << "%" << endl;
    
    // Clean up
    delete c1;
    delete c2;
    delete c3;
    
    noDeadtimeFile->Close();
    deadtimeFile->Close();
    
    cout << "\nSummary:" << endl;
    cout << "- Outputs saved to: " << endl;
    cout << "  - " << noDeadtimeOutFileName << endl;
    cout << "  - " << deadtimeOutFileName << endl;
    cout << "  - " << ratioOutFileName << endl;
    
    return 0;
}
