void PlotBinCountDistribution() {

    TFile *f = TFile::Open("mc_e+_job_rod43_layer42_run43_1_Test_20evt_e+_100_100.root");
    

    TTree *tree = (TTree*)f->Get("tree;4");
    

    int binNumbers[] = {40, 53, 80, 160, 400, 4000};
    int channelareas[] = {100, 75, 50, 25, 10, 1};
    int numConfigs = sizeof(binNumbers)/sizeof(binNumbers[0]);
    

    TCanvas *c1 = new TCanvas("c1", "Normalized Bin Count Distribution", 1200, 800);
    c1->SetLogy();
    

    gStyle->SetOptStat(0);
    

    TLegend *leg = new TLegend(0.65, 0.65, 0.88, 0.88);
    leg->SetHeader("Channel areas");
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    

    int colors[] = {kRed, kBlue, kGreen, kGray, kOrange, kBlack};
    

    vector<map<int, double>> allCountFrequencies; 
    

    int overallMaxCount = 0;
    

    for (int i = 0; i < numConfigs; i++) {
        int nBins = binNumbers[i];
        

        gROOT->cd();
        tree->Draw(Form("OP_pos_final_y:OP_pos_final_x>>h2_%d(%d,-4.3,-3.9,%d,4.3,4.7)", 
                        nBins, nBins, nBins), 
                  "OP_pos_final_z>80 && OP_isCoreC", "goff");
        

        TH2F *h2 = (TH2F*)gDirectory->Get(Form("h2_%d", nBins));
        

        map<int, int> tempFrequency;
        

        int totalBins = 0;
        int nonEmptyBins = 0;


        for (int binX = 1; binX <= nBins; binX++) {
            for (int binY = 1; binY <= nBins; binY++) {
                int content = (int)h2->GetBinContent(binX, binY);
                tempFrequency[content]++;
                if (content > overallMaxCount) overallMaxCount = content;
                if (content > 0) {
                    nonEmptyBins++;
                }
                totalBins++;
            }
        }
        

        double fillPercentage = 100.0 * nonEmptyBins / totalBins;
        
        int maxFrequency = 0;
        for (auto& pair : tempFrequency) {
            if (pair.first != -1 && pair.second > maxFrequency) {
                maxFrequency = pair.second;
            }
        }
        

        map<int, double> countFrequency;
        
        countFrequency[-1] = fillPercentage;
        

        for (auto& pair : tempFrequency) {
            if (pair.first >= 0) {
                countFrequency[pair.first] = (double)pair.second / maxFrequency;
            }
        }
        
        allCountFrequencies.push_back(countFrequency);
        
        delete h2;
    }
    

    vector<TH1F*> countHistograms;
    
    for (int i = 0; i < numConfigs; i++) {
        map<int, double>& countFrequency = allCountFrequencies[i];
        double fillPercentage = countFrequency[-1];
        
        countFrequency.erase(-1);
        
        TH1F *hist = new TH1F(Form("hCounts_%d", binNumbers[i]),
                            Form("%d x %d#mu m^{2} (%.1f%% filled)", channelareas[i], channelareas[i], fillPercentage),
                            overallMaxCount+1, -0.5, overallMaxCount+0.5);
        
        for (auto& pair : countFrequency) {
            int count = pair.first;
            double normalizedFrequency = pair.second;
            

            hist->SetBinContent(count+1, normalizedFrequency);
        }
        
        hist->SetLineColor(colors[i]);
        hist->SetLineWidth(2);
        
        leg->AddEntry(hist, hist->GetTitle(), "l");
        
        hist->SetTitle("Normalized Distribution of NPhotons Per Channel");
        hist->GetXaxis()->SetTitle("NPhotons Per Channel");
        hist->GetYaxis()->SetTitle("Normalized Frequency");
        
        countHistograms.push_back(hist);
    }
    
    for (int i = 0; i < countHistograms.size(); i++) {
        countHistograms[i]->GetYaxis()->SetRangeUser(0.0000001, 10);
        
        countHistograms[i]->GetXaxis()->SetRangeUser(-0.5, min(100.0, (double)overallMaxCount));
        
        if (i == 0) {
            countHistograms[i]->Draw("hist");
        } else {
            countHistograms[i]->Draw("hist same");
        }
    }
    
    leg->Draw();
    
    c1->Update();
    
    c1->SaveAs("bin_count_distribution_normalized_by_max.png");
    
    cout << "Analysis complete. Check bin_count_distribution_normalized_by_max.png" << endl;
}
