/*
This code plots event display for particle tracking data from ROOT files.
It creates three subplots: Y vs X (final position), X vs Z (produced position), and Y vs Z (produced position).

Usage:
root -l
.L event_display.C
plotEvent(897, 1, "pi-_E10-10_1000_0/")

Parameters: eventIndex, applyZCut (1=yes, 0=no), inputDirectory
*/

#include <TFile.h>
#include <TTree.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TPad.h>
#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>

void plotEvent(int eventIndex = 646, bool applyZCut = true, std::string outputDir = "pi-_E10-10_1000_0/") {
    
    struct stat info;
    if (stat("pic_eventdisplay", &info) != 0) {
        system("mkdir -p pic_eventdisplay");
    }
    
    int fileNumber = eventIndex / 10 + 1;
    int eventNumber = eventIndex % 10;
    
    std::string fileName = outputDir + "mc_pi-_job_run1_" + std::to_string(fileNumber) + "_Test_10evt_pi-_10_10.root";
    
    std::cout << "Processing event " << eventIndex << std::endl;
    std::cout << "File: " << fileName << std::endl;
    std::cout << "Event number in file: " << eventNumber << std::endl;
    
    TFile* file = TFile::Open(fileName.c_str());
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file " << fileName << std::endl;
        return;
    }
    
    TTree* tree = (TTree*)file->Get("tree");
    if (!tree) {
        std::cerr << "Error: Cannot find tree in file" << std::endl;
        file->Close();
        return;
    }
    
    std::vector<float>* OP_pos_final_x = nullptr;
    std::vector<float>* OP_pos_final_y = nullptr;
    std::vector<float>* OP_pos_final_z = nullptr;
    std::vector<float>* OP_pos_produced_x = nullptr;
    std::vector<float>* OP_pos_produced_y = nullptr;
    std::vector<float>* OP_pos_produced_z = nullptr;
    
    tree->SetBranchAddress("OP_pos_final_x", &OP_pos_final_x);
    tree->SetBranchAddress("OP_pos_final_y", &OP_pos_final_y);
    tree->SetBranchAddress("OP_pos_final_z", &OP_pos_final_z);
    tree->SetBranchAddress("OP_pos_produced_x", &OP_pos_produced_x);
    tree->SetBranchAddress("OP_pos_produced_y", &OP_pos_produced_y);
    tree->SetBranchAddress("OP_pos_produced_z", &OP_pos_produced_z);
    
    tree->GetEntry(eventNumber);
    
    std::string xyTitle = applyZCut ? "Y vs X (Final Position)" : "Y vs X (Produced Position)";
    TH2F* hist_xy = new TH2F("hist_xy", xyTitle.c_str(), 
                            144, -18, 18,
                            128, -16, 16);
    
    TH2F* hist_xz = new TH2F("hist_xz", "X vs Z (Produced Position)", 
                            200, -100, 100,
                            144, -18, 18);
    
    TH2F* hist_yz = new TH2F("hist_yz", "Y vs Z (Produced Position)", 
                            200, -100, 100,
                            128, -16, 16);
    
    int nEntries = OP_pos_final_x->size();
    std::cout << "Number of entries in event: " << nEntries << std::endl;
    
    int usedEntries = 0;
    for (int i = 0; i < nEntries; i++) {
        float final_x = (*OP_pos_final_x)[i];
        float final_y = (*OP_pos_final_y)[i];
        float final_z = (*OP_pos_final_z)[i];
        float prod_x = (*OP_pos_produced_x)[i];
        float prod_y = (*OP_pos_produced_y)[i];
        float prod_z = (*OP_pos_produced_z)[i];
        
        if (applyZCut) {
            if (final_z > 80) {
                hist_xy->Fill(final_x, final_y);
                usedEntries++;
            }
        } else {
            hist_xy->Fill(prod_x, prod_y);
            usedEntries++;
        }
        
        hist_xz->Fill(prod_z, prod_x);
        hist_yz->Fill(prod_z, prod_y);
    }
    
    std::cout << "Used entries: " << usedEntries << std::endl;
    
    gStyle->SetOptStat(0);
    gStyle->SetPalette(1);
    
    TCanvas* canvas = new TCanvas("canvas", "Event Display", 1400, 1000);
    
    TPad* pad1 = new TPad("pad1", "Y vs X", 0.0, 0.0, 0.55, 1.0);
    TPad* pad2 = new TPad("pad2", "X vs Z", 0.55, 0.5, 1.0, 1.0);
    TPad* pad3 = new TPad("pad3", "Y vs Z", 0.55, 0.0, 1.0, 0.5);
    
    pad1->SetLeftMargin(0.12);
    pad1->SetRightMargin(0.15);
    pad1->SetBottomMargin(0.12);
    pad1->SetTopMargin(0.1);
    
    pad2->SetLeftMargin(0.15);
    pad2->SetRightMargin(0.15);
    pad2->SetBottomMargin(0.15);
    pad2->SetTopMargin(0.1);
    
    pad3->SetLeftMargin(0.15);
    pad3->SetRightMargin(0.15);
    pad3->SetBottomMargin(0.15);
    pad3->SetTopMargin(0.1);
    
    pad1->Draw();
    pad2->Draw();
    pad3->Draw();
    
    pad1->cd();
    std::string mainTitle = applyZCut ? 
        Form("Event %d: Y vs X (Final Position) [Z > 80]", eventIndex) :
        Form("Event %d: Y vs X (Produced Position)", eventIndex);
    hist_xy->SetTitle(mainTitle.c_str());
    hist_xy->GetXaxis()->SetTitle("X [cm]");
    hist_xy->GetYaxis()->SetTitle("Y [cm]");
    hist_xy->GetXaxis()->SetTitleSize(0.05);
    hist_xy->GetYaxis()->SetTitleSize(0.05);
    hist_xy->GetXaxis()->SetRangeUser(-18, 18);
    hist_xy->GetYaxis()->SetRangeUser(-16, 16);
    hist_xy->Draw("COLZ");
    
    pad2->cd();
    hist_xz->SetTitle(Form("Event %d: X vs Z (Produced)", eventIndex));
    hist_xz->GetXaxis()->SetTitle("Z [cm]");
    hist_xz->GetYaxis()->SetTitle("X [cm]");
    hist_xz->GetXaxis()->SetTitleSize(0.06);
    hist_xz->GetYaxis()->SetTitleSize(0.06);
    hist_xz->GetXaxis()->SetLabelSize(0.05);
    hist_xz->GetYaxis()->SetLabelSize(0.05);
    hist_xz->GetXaxis()->SetRangeUser(-100, 100);
    hist_xz->GetYaxis()->SetRangeUser(-18, 18);
    hist_xz->Draw("COLZ");
    
    pad3->cd();
    hist_yz->SetTitle(Form("Event %d: Y vs Z (Produced)", eventIndex));
    hist_yz->GetXaxis()->SetTitle("Z [cm]");
    hist_yz->GetYaxis()->SetTitle("Y [cm]");
    hist_yz->GetXaxis()->SetTitleSize(0.06);
    hist_yz->GetYaxis()->SetTitleSize(0.06);
    hist_yz->GetXaxis()->SetLabelSize(0.05);
    hist_yz->GetYaxis()->SetLabelSize(0.05);
    hist_yz->GetXaxis()->SetRangeUser(-100, 100);
    hist_yz->GetYaxis()->SetRangeUser(-16, 16);
    hist_yz->Draw("COLZ");
    
    std::string outputFileName = "pic_eventdisplay/event_" + std::to_string(eventIndex) + 
                                (applyZCut ? "_zcut" : "_nozcut") + ".png";
    canvas->SaveAs(outputFileName.c_str());
    
    std::cout << "Plot saved as: " << outputFileName << std::endl;
    
    delete hist_xy;
    delete hist_xz;
    delete hist_yz;
    delete canvas;
    file->Close();
    delete file;
}
