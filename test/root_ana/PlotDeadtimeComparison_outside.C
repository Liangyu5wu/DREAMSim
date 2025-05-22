#include <iostream>
#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"
#include "TH1F.h"
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

// root -l
// .L PlotDeadtimeComparison.C
// PlotDeadtimeComparison(50,30,"../deadtimedata/", 0.5,0.2)


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
            
            double binX = xMin + (i - 0.5) * binWidth_x;
            double binY = yMin + (j - 0.5) * binWidth_y;

            double dx = binX - centerX;
            double dy = binY - centerY;
            double distance = sqrt(dx*dx + dy*dy);
            
            if (distance <= radius) {
                countsInCircle += binContent;
            }
        }
    }
    
    return (totalCounts > 0) ? countsInCircle / totalCounts : 0.0;
}

double findRadiusForTargetRatio(TH2F* hist, double centerX, double centerY, double targetRatio, double startRadius = 0.04, double minRadius = 0.01, double step = 0.0001) {
    double radius = startRadius;
    double ratio = calculateCircleRatio(hist, centerX, centerY, radius);
    
    cout << "Starting search with radius: " << radius << ", ratio: " << ratio * 100 << "%" << endl;
    
    while (ratio > targetRatio && radius > minRadius) {
        radius -= step;
        ratio = calculateCircleRatio(hist, centerX, centerY, radius);
    }
    
    cout << "Found radius: " << radius << " with ratio: " << ratio * 100 << "%" << endl;
    return radius;
}

void collectRatioValues(TH2F* ratioHist, double centerX, double centerY, double radius1, double radius2, 
                       vector<double>& allValues, vector<double>& circle1Values, vector<double>& circle2Values) {
    
    int xBins = ratioHist->GetNbinsX();
    int yBins = ratioHist->GetNbinsY();
    
    double xMin = ratioHist->GetXaxis()->GetXmin();
    double xMax = ratioHist->GetXaxis()->GetXmax();
    double yMin = ratioHist->GetYaxis()->GetXmin();
    double yMax = ratioHist->GetYaxis()->GetXmax();
    
    double binWidth_x = (xMax - xMin) / xBins;
    double binWidth_y = (yMax - yMin) / yBins;
    
    for (int i = 1; i <= xBins; i++) {
        for (int j = 1; j <= yBins; j++) {
            double ratio = ratioHist->GetBinContent(i, j);
            if (ratio <= 0) continue;
            
            double binX = xMin + (i - 0.5) * binWidth_x;
            double binY = yMin + (j - 0.5) * binWidth_y;
            
            double dx = binX - centerX;
            double dy = binY - centerY;
            double distance = sqrt(dx*dx + dy*dy);
            
            allValues.push_back(ratio);

            if (distance <= radius2) {
                circle2Values.push_back(ratio);
            }
            
            if (distance <= radius1) {
                circle1Values.push_back(ratio);
            }
        }
    }
}

void collectDetailedRatioValues(TH2F* ratioHist, double centerX, double centerY, double radius1, double radius2, 
                       vector<double>& allValues, vector<double>& circle1outValues, 
                       vector<double>& circle1inValues, vector<double>& circle2Values) {
    
    int xBins = ratioHist->GetNbinsX();
    int yBins = ratioHist->GetNbinsY();
    
    double xMin = ratioHist->GetXaxis()->GetXmin();
    double xMax = ratioHist->GetXaxis()->GetXmax();
    double yMin = ratioHist->GetYaxis()->GetXmin();
    double yMax = ratioHist->GetYaxis()->GetXmax();
    
    double binWidth_x = (xMax - xMin) / xBins;
    double binWidth_y = (yMax - yMin) / yBins;
    
    for (int i = 1; i <= xBins; i++) {
        for (int j = 1; j <= yBins; j++) {
            double ratio = ratioHist->GetBinContent(i, j);
            if (ratio <= 0) continue;
            
            double binX = xMin + (i - 0.5) * binWidth_x;
            double binY = yMin + (j - 0.5) * binWidth_y;
            
            double dx = binX - centerX;
            double dy = binY - centerY;
            double distance = sqrt(dx*dx + dy*dy);
            
            allValues.push_back(ratio);

            if (distance <= radius2) {
                circle2Values.push_back(ratio);
            } else if (distance <= radius1) {
                circle1inValues.push_back(ratio);
            } else {
                circle1outValues.push_back(ratio);
            }
        }
    }
}





int PlotDeadtimeComparison(
    int pixelSize = 100,            // Pixel size (20, 25, 50, 100)
    double deadtime = 30.0,         // Deadtime value in ns (0.0 for NoDeadtime, or 5.0, 10.0, 30.0)
    const char* baseDir = "../deadtimedata/",  // Directory containing the ROOT files
    double circleRatio1 = 0.20,     // First circle target ratio (default 20%)
    double circleRatio2 = 0.15      // Second circle target ratio (default 15%)
) {

    if (pixelSize != 20 && pixelSize != 25 && 
        pixelSize != 50 && pixelSize != 100) {
        cout << "Pixel size must be one of: 20, 25, 50, 100" << endl;
        return 1;
    }
    
    if (circleRatio1 <= 0 || circleRatio1 >= 1 || circleRatio2 <= 0 || circleRatio2 >= 1) {
        cout << "Circle ratios must be between 0 and 1" << endl;
        return 1;
    }
    
    double circlePercent1 = circleRatio1 * 100;
    double circlePercent2 = circleRatio2 * 100;
    
    int actualPixelSize = 1000 / pixelSize;
    
    TString noDeadtimeFileName;
    TString deadtimeFileName;
    
    noDeadtimeFileName.Form("%s2DHistogram_NoDeadtime_%dx%d_100files_2000events.root", 
                           baseDir, pixelSize, pixelSize);
    
    if (deadtime <= 0.0) {
        deadtimeFileName = noDeadtimeFileName;
    } else {
        deadtimeFileName.Form("%s2DHistogram_Deadtime%.1fns_%dx%d_100files_2000events.root", 
                             baseDir, deadtime, pixelSize, pixelSize);
    }
    
    cout << "Processing files:" << endl;
    cout << "- No deadtime: " << noDeadtimeFileName << endl;
    cout << "- With deadtime: " << deadtimeFileName << endl;
    cout << "- Circle targets: " << circlePercent1 << "% and " << circlePercent2 << "%" << endl;
    
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
    
    // Calculate ratios for deadtime histogram
    double deadtimeRatio1 = calculateCircleRatio(deadtimeHistClone, centerX, centerY, radius1);
    double deadtimeRatio2 = calculateCircleRatio(deadtimeHistClone, centerX, centerY, radius2);
    
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
    
    TLegend* legend1 = new TLegend(0.10, 0.75, 0.40, 0.90);
    legend1->SetBorderSize(0);
    legend1->SetFillColor(0);
    legend1->SetFillStyle(0); 
    legend1->AddEntry(circle1, circle1Text, "l");
    legend1->AddEntry (circle2, circle2Text, "l");
    legend1->Draw();
    
    TString noDeadtimeOutFileName = Form("NoDeadtime_%dx%d_withCircles_%.0f_%.0f.png", 
                                        pixelSize, pixelSize, circlePercent1, circlePercent2);
    c1->SaveAs(noDeadtimeOutFileName.Data());
    
    // Draw deadtime canvas with circles
    TCanvas* c2 = new TCanvas("c2", "With Deadtime", 800, 700);
    deadtimeHistClone->Draw("colz");
    
    // Draw circles on deadtime histogram too
    TEllipse* deadtimeCircle1 = new TEllipse(centerX, centerY, radius1, radius1);
    deadtimeCircle1->SetFillStyle(0);
    deadtimeCircle1->SetLineColor(kBlue);
    deadtimeCircle1->SetLineWidth(2);
    deadtimeCircle1->Draw();
    
    TEllipse* deadtimeCircle2 = new TEllipse(centerX, centerY, radius2, radius2);
    deadtimeCircle2->SetFillStyle(0);
    deadtimeCircle2->SetLineColor(kRed);
    deadtimeCircle2->SetLineWidth(2);
    deadtimeCircle2->Draw();
    
    // Add text for circles on deadtime histogram
    TString deadtimeCircle1Text = Form("R = %.4f cm (%.1f%%)", radius1, deadtimeRatio1 * 100);
    TString deadtimeCircle2Text = Form("R = %.4f cm (%.1f%%)", radius2, deadtimeRatio2 * 100);
    
    TLegend* legend2 = new TLegend(0.10, 0.75, 0.40, 0.90);
    legend2->SetBorderSize(0);
    legend2->SetFillColor(0);
    legend2->SetFillStyle(0);
    legend2->AddEntry(deadtimeCircle1, deadtimeCircle1Text, "l");
    legend2->AddEntry(deadtimeCircle2, deadtimeCircle2Text, "l");
    legend2->Draw();
    
    TString deadtimeOutFileName = Form("Deadtime%.1fns_%dx%d_withCircles.png", deadtime, pixelSize, pixelSize);
    c2->SaveAs(deadtimeOutFileName.Data());
    
    // Draw ratio canvas
    TCanvas* c3 = new TCanvas("c3", "Ratio", 800, 700);
    ratioHist->Draw("colz");
    
    TString ratioOutFileName = Form("ReceptionRatio_Deadtime%.1fns_%dx%d.png", deadtime, pixelSize, pixelSize);
    c3->SaveAs(ratioOutFileName.Data());
    
    // Collect ratio values for different regions
    vector<double> allRatioValues;
    vector<double> circle1outRatioValues;
    vector<double> circle1inRatioValues;
    vector<double> circle2RatioValues;
    
    // collectRatioValues(ratioHist, centerX, centerY, radius1, radius2, 
    //               allRatioValues, circle1RatioValues, circle2RatioValues);
    collectDetailedRatioValues(ratioHist, centerX, centerY, radius1, radius2, allRatioValues, circle1outRatioValues, circle1inRatioValues, circle2RatioValues);
    
    // Create histograms for the ratio distributions
    double ratioMin = 0.65;
    double ratioMax = 1.1;
    int ratioBins = 50;

    
    TH1F* allRatioHist = new TH1F("allRatioHist", "All Regions", ratioBins, ratioMin, ratioMax);
    TH1F* circle1outRatioHist = new TH1F("circle1outRatioHist", "Outside Blue Circle", ratioBins, ratioMin, ratioMax);
    TH1F* circle1inRatioHist = new TH1F("circle1inRatioHist", "Inside Blue Outside Red Circle", ratioBins, ratioMin, ratioMax);
    TH1F* circle2RatioHist = new TH1F("circle2RatioHist", "Inside Red Circle", ratioBins, ratioMin, ratioMax);

    for (double val : allRatioValues) allRatioHist->Fill(val);
    for (double val : circle1outRatioValues) circle1outRatioHist->Fill(val);
    for (double val : circle1inRatioValues) circle1inRatioHist->Fill(val);
    for (double val : circle2RatioValues) circle2RatioHist->Fill(val);

    // Set histogram styles
    allRatioHist->SetLineColor(kYellow);
    allRatioHist->SetLineWidth(2);

    circle1outRatioHist->SetLineColor(kBlack);
    circle1outRatioHist->SetLineWidth(2);

    circle1inRatioHist->SetLineColor(kBlue);
    circle1inRatioHist->SetLineWidth(2);

    circle2RatioHist->SetLineColor(kRed);
    circle2RatioHist->SetLineWidth(2);

    double allMean = 0.0, allRMS = 0.0;
    double circle1outMean = 0.0, circle1outRMS = 0.0;
    double circle1inMean = 0.0, circle1inRMS = 0.0;
    double circle2Mean = 0.0, circle2RMS = 0.0;

    if (!allRatioValues.empty()) {
        double sum = 0.0;
        for (double val : allRatioValues) sum += val;
        allMean = sum / allRatioValues.size();
        
        sum = 0.0;
        for (double val : allRatioValues) sum += (val - allMean) * (val - allMean);
        allRMS = sqrt(sum / allRatioValues.size());
    }

    if (!circle1outRatioValues.empty()) {
        double sum = 0.0;
        for (double val : circle1outRatioValues) sum += val;
        circle1outMean = sum / circle1outRatioValues.size();
        
        sum = 0.0;
        for (double val : circle1outRatioValues) sum += (val - circle1outMean) * (val - circle1outMean);
        circle1outRMS = sqrt(sum / circle1outRatioValues.size());
    }

    if (!circle1inRatioValues.empty()) {
        double sum = 0.0;
        for (double val : circle1inRatioValues) sum += val;
        circle1inMean = sum / circle1inRatioValues.size();
        
        sum = 0.0;
        for (double val : circle1inRatioValues) sum += (val - circle1inMean) * (val - circle1inMean);
        circle1inRMS = sqrt(sum / circle1inRatioValues.size());
    }

    if (!circle2RatioValues.empty()) {
        double sum = 0.0;
        for (double val : circle2RatioValues) sum += val;
        circle2Mean = sum / circle2RatioValues.size();
        
        sum = 0.0;
        for (double val : circle2RatioValues) sum += (val - circle2Mean) * (val - circle2Mean);
        circle2RMS = sqrt(sum / circle2RatioValues.size());
    }
        
    // Calculate max height for scaling
    double maxHeight = max(max(allRatioHist->GetMaximum(), circle1outRatioHist->GetMaximum()), max(circle1inRatioHist->GetMaximum(), 
                      circle2RatioHist->GetMaximum()));
    
    // Create canvas for ratio distribution
    TCanvas* c4 = new TCanvas("c4", "Ratio Distribution", 900, 700);
    c4->SetLogy(1);
    
    // Set histogram properties
    circle1outRatioHist->SetTitle(Form("Reception Ratio Distribution - %dx%d #mu m^{2}, Deadtime %.1f ns", 
                         actualPixelSize, actualPixelSize, deadtime));
    circle1outRatioHist->SetXTitle("Reception Ratio (Deadtime/No Deadtime)");
    circle1outRatioHist->SetYTitle("Frequency");
    circle1outRatioHist->SetMaximum(maxHeight * 5);
    circle1outRatioHist->SetMinimum(0.5);

    circle1outRatioHist->Draw("hist");
    circle1inRatioHist->Draw("hist same");
    circle2RatioHist->Draw("hist same");

    // Add legend
    TLegend* legend4 = new TLegend(0.12, 0.60, 0.59, 0.9);
    legend4->SetBorderSize(0);
    legend4->SetFillColor(0);
    legend4->SetFillStyle(0);
    legend4->SetTextSize(0.03);


    legend4->AddEntry(circle1outRatioHist, 
                    Form("Outside Blue Circle (Mean=%.4f, RMS=%.4f)", circle1outMean, circle1outRMS), 
                    "l");
    legend4->AddEntry(circle1inRatioHist, 
                    Form("Inside Blue Circle, Outside Red Circle (Mean=%.4f, RMS=%.4f)", circle1inMean, circle1inRMS), 
                    "l");
    legend4->AddEntry(circle2RatioHist, 
                    Form("Inside Red Circle (Mean=%.4f, RMS=%.4f)", circle2Mean, circle2RMS), 
                    "l");

    legend4->Draw();
    
    // Save distribution canvas
    TString distOutFileName = Form("RatioDistribution_Deadtime%.1fns_%dx%d.png", 
                                  deadtime, pixelSize, pixelSize);
    c4->SaveAs(distOutFileName.Data());
    
    // Save histograms to ROOT file
    TString rootOutFileName = Form("RatioHistograms_Deadtime%.1fns_%dx%d.root", 
                              deadtime, pixelSize, pixelSize);
    TFile* outFile = new TFile(rootOutFileName.Data(), "RECREATE");
    
    // Save histograms
    allRatioHist->Write("AllRegions");
    circle1outRatioHist->Write("OutsideCircle1");
    circle1inRatioHist->Write("InsideCircle1_OutsideCircle2");
    circle2RatioHist->Write("InsideCircle2");

    
    TH2F* noDeadtimeHistCopy = (TH2F*)noDeadtimeHist->Clone("NoDeadtimeHist");
    TH2F* deadtimeHistCopy = (TH2F*)deadtimeHist->Clone("DeadtimeHist");
    TH2F* ratioHistCopy = (TH2F*)ratioHist->Clone("RatioHist");
    
    noDeadtimeHistCopy->Write();
    deadtimeHistCopy->Write();
    ratioHistCopy->Write();
    
    
    outFile->Close();
    
    // Calculate rejection rate for output
    double totalEntries_noDeadtime = noDeadtimeHistClone->GetSumOfWeights();
    double totalEntries_deadtime = deadtimeHistClone->GetSumOfWeights();
    
    cout << "\nTotal entries in No Deadtime histogram: " << totalEntries_noDeadtime << endl;
    cout << "Total entries in Deadtime histogram: " << totalEntries_deadtime << endl;
    cout << "Rejection rate: " << 100.0 * (1.0 - totalEntries_deadtime / totalEntries_noDeadtime) << "%" << endl;
    
    // Output circle information again
    cout << "\nCircle Information:" << endl;
    cout << "- " << circlePercent1 << "% Circle (Blue): Radius = " << std::fixed << std::setprecision(4) << radius1 << " cm" << endl; 
    cout << "  No Deadtime Ratio = " << std::setprecision(2) << (actualRatio1 * 100) << "%" << endl;
    cout << "  With Deadtime Ratio = " << std::setprecision(2) << (deadtimeRatio1 * 100) << "%" << endl;
    
    cout << "- " << circlePercent2 << "% Circle (Red): Radius = " << std::fixed << std::setprecision(4) << radius2 << " cm" << endl;
    cout << "  No Deadtime Ratio = " << std::setprecision(2) << (actualRatio2 * 100) << "%" << endl;
    cout << "  With Deadtime Ratio = " << std::setprecision(2) << (deadtimeRatio2 * 100) << "%" << endl;
    
    cout << "\nRatio Distribution Statistics (excluding ratio=1):" << endl;
    cout << "- All Regions: " << allRatioValues.size() << " values " << " total), Mean=" << allMean << ", RMS=" << allRMS << endl;
    cout << "- Outside Blue Circle: " << circle1outRatioValues.size() << " values " << " total), Mean=" << circle1outMean << ", RMS=" << circle1outRMS << endl;
    cout << "- Inside Blue Circle Outside Red Circle: " << circle1inRatioValues.size() << " values " << " total), Mean=" << circle1inMean << ", RMS=" << circle1inRMS << endl;
    cout << "- Inside Red Circle: " << circle2RatioValues.size() << " values " << " total), Mean=" << circle2Mean << ", RMS=" << circle2RMS << endl;
    
    // Clean up
    delete c1;
    delete c2;
    delete c3;
    delete c4;
    
    noDeadtimeFile->Close();
    deadtimeFile->Close();

    cout << "\nSummary:" << endl;
    cout << "- Outputs saved to: " << endl;
    cout << "  - " << noDeadtimeOutFileName << endl;
    cout << "  - " << deadtimeOutFileName << endl;
    cout << "  - " << ratioOutFileName << endl;
    cout << "  - " << distOutFileName << endl;
    cout << "  - " << rootOutFileName << " (ROOT file with histograms)" << endl;
    
    return 0;
}
