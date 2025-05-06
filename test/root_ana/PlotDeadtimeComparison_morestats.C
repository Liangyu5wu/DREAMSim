#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TDirectory.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TEllipse.h"
#include "TLegend.h"
#include "TText.h"
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include "TString.h"
#include <iomanip>

using namespace std;

// Function to calculate the ratio of counts inside a circle to total counts
double calculateCircleRatio(TH2F* hist, double centerX, double centerY, double radius) {
    double countsInCircle = 0.0;
    double totalCounts = hist->GetSumOfWeights();
    
    int xBins = hist->GetNbinsX();
    int yBins = hist->GetNbinsY();
    
    double xMin = hist->GetXaxis()->GetXmin();
    double xMax = hist->GetXaxis()->GetXmax();
    double yMin = hist->GetYaxis()->GetXmin();
    double yMax = hist->GetYaxis()->GetXmax();
    
    double binWidth_x = (xMax - xMin) / xBins;
    double binWidth_y = (yMax - yMin) / yBins;
    
    for (int i = 1; i <= xBins; i++) {
        for (int j = 1; j <= yBins; j++) {
            double binContent = hist->GetBinContent(i, j);
            if (binContent <= 0) continue;
            
            // Get bin center
            double binX = xMin + (i - 0.5) * binWidth_x;
            double binY = yMin + (j - 0.5) * binWidth_y;
            
            // Calculate distance from circle center
            double dx = binX - centerX;
            double dy = binY - centerY;
            double distance = sqrt(dx*dx + dy*dy);
            
            // Add counts if inside circle
            if (distance <= radius) {
                countsInCircle += binContent;
            }
        }
    }
    
    return (totalCounts > 0) ? countsInCircle / totalCounts : 0.0;
}

// Function to find radius for a target ratio
double findRadiusForTargetRatio(TH2F* hist, double centerX, double centerY, double targetRatio, double startRadius = 0.04, double minRadius = 0.01, double step = 0.0001) {
    double radius = startRadius;
    double ratio = calculateCircleRatio(hist, centerX, centerY, radius);
    
    cout << "Starting search with radius: " << radius << ", ratio: " << ratio * 100 << "%" << endl;
    
    // Reduce radius until we reach the target ratio
    while (ratio > targetRatio && radius > minRadius) {
        radius -= step;
        ratio = calculateCircleRatio(hist, centerX, centerY, radius);
    }
    
    cout << "Found radius: " << radius << " with ratio: " << ratio * 100 << "%" << endl;
    return radius;
}

int PlotDeadtimeComparison(
    int pixelSize = 100,            // Pixel size (20, 25, 40, 50, 80, 100)
    double deadtime = 30.0,         // Deadtime value in ns (0.0 for NoDeadtime, or 1.0, 2.0, 5.0, 10.0, 30.0)
    const char* baseDir = "../deadtimedata/",  // Directory containing the ROOT files
    double circleRatio1 = 0.20,     // First circle target ratio (default 20%)
    double circleRatio2 = 0.15      // Second circle target ratio (default 15%)
) {
    // Validate pixel size
    if (pixelSize != 20 && pixelSize != 25 && pixelSize != 40 && 
        pixelSize != 50 && pixelSize != 80 && pixelSize != 100) {
        cout << "Pixel size must be one of: 20, 25, 40, 50, 80, 100" << endl;
        return 1;
    }
    
    // Validate circle ratios
    if (circleRatio1 <= 0 || circleRatio1 >= 1 || circleRatio2 <= 0 || circleRatio2 >= 1) {
        cout << "Circle ratios must be between 0 and 1" << endl;
        return 1;
    }
    
    // Format circle percentages for display
    double circlePercent1 = circleRatio1 * 100;
    double circlePercent2 = circleRatio2 * 100;
    
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
    cout << "- Circle targets: " << circlePercent1 << "% and " << circlePercent2 << "%" << endl;
    
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
    
    // Define circle center
    double centerX = -4.16;
    double centerY = 4.527;
    
    // Find radii for the specified ratios
    cout << "\nSearching for " << circlePercent1 << "% circle radius..." << endl;
    double radius1 = findRadiusForTargetRatio(noDeadtimeHistClone, centerX, centerY, circleRatio1);
    double actualRatio1 = calculateCircleRatio(noDeadtimeHistClone, centerX, centerY, radius1);
    
    cout << "\nSearching for " << circlePercent2 << "% circle radius..." << endl;
    double radius2 = findRadiusForTargetRatio(noDeadtimeHistClone, centerX, centerY, circleRatio2);
    double actualRatio2 = calculateCircleRatio(noDeadtimeHistClone, centerX, centerY, radius2);
    
    // Draw no deadtime canvas with circles
    TCanvas* c1 = new TCanvas("c1", "No Deadtime", 800, 700);
    noDeadtimeHistClone->Draw("colz");
    
    // Draw first circle
    TEllipse* circle1 = new TEllipse(centerX, centerY, radius1, radius1);
    circle1->SetFillStyle(0);
    circle1->SetLineColor(kBlue);
    circle1->SetLineWidth(2);
    circle1->Draw();
    
    // Draw second circle
    TEllipse* circle2 = new TEllipse(centerX, centerY, radius2, radius2);
    circle2->SetFillStyle(0);
    circle2->SetLineColor(kRed);
    circle2->SetLineWidth(2);
    circle2->Draw();
    
    // Add text for circle information
    TString circle1Text = Form("R = %.4f cm (%.1f%%)", radius1, actualRatio1 * 100);
    TString circle2Text = Form("R = %.4f cm (%.1f%%)", radius2, actualRatio2 * 100);
    
    TLegend* legend = new TLegend(0.15, 0.75, 0.45, 0.90);
    legend->SetBorderSize(0);
    legend->SetFillColor(0);
    legend->SetFillStyle(0); 
    legend->AddEntry(circle1, circle1Text, "l");
    legend->AddEntry(circle2, circle2Text, "l");
    legend->Draw();
    
    TString noDeadtimeOutFileName = Form("NoDeadtime_%dx%d_withCircles_%.0f_%.0f.png", 
                                        pixelSize, pixelSize, circlePercent1, circlePercent2);
    c1->SaveAs(noDeadtimeOutFileName.Data());
    
    // Draw deadtime canvas (without circles)
    TCanvas* c2 = new TCanvas("c2", "With Deadtime", 800, 700);
    deadtimeHistClone->Draw("colz");
    
    TString deadtimeOutFileName = Form("Deadtime%.1fns_%dx%d.png", deadtime, pixelSize, pixelSize);
    c2->SaveAs(deadtimeOutFileName.Data());
    
    // Draw ratio canvas
    TCanvas* c3 = new TCanvas("c3", "Ratio", 800, 700);
    ratioHist->Draw("colz");
    
    TString ratioOutFileName = Form("ReceptionRatio_Deadtime%.1fns_%dx%d.png", deadtime, pixelSize, pixelSize);
    c3->SaveAs(ratioOutFileName.Data());
    
    // Calculate rejection rate for output
    double totalEntries_noDeadtime = noDeadtimeHistClone->GetSumOfWeights();
    double totalEntries_deadtime = deadtimeHistClone->GetSumOfWeights();
    
    cout << "\nTotal entries in No Deadtime histogram: " << totalEntries_noDeadtime << endl;
    cout << "Total entries in Deadtime histogram: " << totalEntries_deadtime << endl;
    cout << "Rejection rate: " << 100.0 * (1.0 - totalEntries_deadtime / totalEntries_noDeadtime) << "%" << endl;
    
    // Output circle information again
    cout << "\nCircle Information:" << endl;
    cout << "- " << circlePercent1 << "% Circle: Radius = " << std::fixed << std::setprecision(4) << radius1 
         << " cm, Actual ratio = " << std::setprecision(2) << (actualRatio1 * 100) << "%" << endl;
    cout << "- " << circlePercent2 << "% Circle: Radius = " << std::fixed << std::setprecision(4) << radius2 
         << " cm, Actual ratio = " << std::setprecision(2) << (actualRatio2 * 100) << "%" << endl;
    
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
