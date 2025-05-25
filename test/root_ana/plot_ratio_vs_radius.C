// plot_ratio_vs_radius.C
// This code processes ROOT files from ana_datas/ directory with naming pattern like 
// RatioHistograms_Deadtime0.0ns_100x100.root for various deadtimes and grid sizes.
// It extracts the RatioHist 2D histogram from each file and plots the reception ratio
// vs distance from center point (-4.16, 4.527).
// Output: RatioVsRadius.root containing individual graphs and a summary canvas.
// Usage: root -l 'plot_ratio_vs_radius.C'

#include "TFile.h"
#include "TH2D.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TString.h"
#include "TSystem.h"
#include "TMath.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TObjArray.h"
#include <iostream>
#include <vector>
#include <string>

void plot_ratio_vs_radius() {
    double centerX = -4.16;
    double centerY = 4.527;
    
    double maxDistance = 0.05;
    
    std::vector<double> deadtimes = {0.0, 5.0, 10.0, 30.0};
    std::vector<int> gridSizes = {100, 50, 25, 20};
    
    TFile *outputFile = new TFile("RatioVsRadius.root", "RECREATE");
    
    TObjArray graphArray;
    
    for (auto deadtime : deadtimes) {
        for (auto gridSize : gridSizes) {
            TString inputFileName = Form("ana_datas/RatioHistograms_Deadtime%.1fns_%dx%d.root", 
                                          deadtime, gridSize, gridSize);
            
            if (gSystem->AccessPathName(inputFileName)) {
                std::cout << "File not found: " << inputFileName << std::endl;
                continue;
            }
            
            TFile *inputFile = TFile::Open(inputFileName);
            if (!inputFile || inputFile->IsZombie()) {
                std::cout << "Error opening file: " << inputFileName << std::endl;
                continue;
            }
            
            TH2D *ratioHist = (TH2D*)inputFile->Get("RatioHist");
            if (!ratioHist) {
                std::cout << "Histogram 'RatioHist' not found in file: " << inputFileName << std::endl;
                inputFile->Close();
                continue;
            }
            
            std::vector<double> distances;
            std::vector<double> ratios;
            
            for (int xBin = 1; xBin <= ratioHist->GetNbinsX(); xBin++) {
                for (int yBin = 1; yBin <= ratioHist->GetNbinsY(); yBin++) {
                    double binContent = ratioHist->GetBinContent(xBin, yBin);
                    
                    if (binContent <= 0) continue;
                    
                    double x = ratioHist->GetXaxis()->GetBinCenter(xBin);
                    double y = ratioHist->GetYaxis()->GetBinCenter(yBin);
                    
                    double distance = TMath::Sqrt((x - centerX) * (x - centerX) + 
                                                 (y - centerY) * (y - centerY));
                    
                    if (distance > maxDistance) continue;
                    
                    distances.push_back(distance);
                    ratios.push_back(binContent);
                }
            }
            
            int nPoints = distances.size();
            if (nPoints > 0) {
                TGraph *graph = new TGraph(nPoints, &distances[0], &ratios[0]);
                TString graphName = Form("RatioVsRadius_Deadtime%.1fns_%dx%d", deadtime, gridSize, gridSize);
                graph->SetName(graphName);
                graph->SetTitle(Form("Ratio vs Distance from Center (%.1f ns, %dx%d)", deadtime, gridSize, gridSize));
                graph->GetXaxis()->SetTitle("Distance from Center (m)");
                graph->GetYaxis()->SetTitle("Reception Ratio");
                
                graph->SetMarkerStyle(20);
                graph->SetMarkerSize(0.5);
                
                outputFile->cd();
                graph->Write();
                
                graphArray.Add(graph);
                
                std::cout << "Created graph for deadtime " << deadtime << " ns, grid size " 
                          << gridSize << "x" << gridSize << " with " << nPoints << " points." << std::endl;
            }
            
            inputFile->Close();
        }
    }
    
    TCanvas *c1 = new TCanvas("c1", "Ratio vs Radius", 1200, 800);
    c1->Divide(2, 2);
    
    for (size_t i = 0; i < deadtimes.size(); i++) {
        c1->cd(i+1);
        
        TLegend *legend = new TLegend(0.7, 0.7, 0.9, 0.9);
        legend->SetHeader(Form("Deadtime %.1f ns", deadtimes[i]));
        
        bool firstPlot = true;
        
        for (int j = 0; j < graphArray.GetEntriesFast(); j++) {
            TGraph *graph = (TGraph*)graphArray.At(j);
            TString graphName = graph->GetName();
            
            if (graphName.Contains(Form("Deadtime%.1fns", deadtimes[i]))) {
                for (auto gridSize : gridSizes) {
                    if (graphName.Contains(Form("%dx%d", gridSize, gridSize))) {
                        int colorIndex = 0;
                        if (gridSize == 100) colorIndex = kBlue;
                        else if (gridSize == 50) colorIndex = kRed;
                        else if (gridSize == 25) colorIndex = kGreen+2;
                        else if (gridSize == 20) colorIndex = kMagenta;
                        
                        graph->SetMarkerColor(colorIndex);
                        graph->SetLineColor(colorIndex);
                        
                        if (firstPlot) {
                            graph->Draw("AP"); // AP option: A=axes, P=points (no connecting lines)
                            firstPlot = false;
                        } else {
                            graph->Draw("P SAME"); // P option: just points (no connecting lines)
                        }
                        
                        legend->AddEntry(graph, Form("%dx%d", gridSize, gridSize), "p");
                        break;
                    }
                }
            }
        }
        
        legend->Draw();
    }
    
    outputFile->cd();
    c1->Write();
    
    c1->SaveAs("RatioVsRadius.png");
    
    outputFile->Close();
    
    std::cout << "Analysis completed. Results saved to RatioVsRadius.root" << std::endl;
}
