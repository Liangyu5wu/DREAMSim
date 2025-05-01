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

using namespace std;

int Plot2dposition(
    int endEventNumber = 0,  // 绘制从事件0到endEventNumber的数据
    const char* fileName = "mc_e+_job_rod43_layer42_run43_1_Test_20evt_e+_100_100.root",
    const char* treeName = "tree;4",
    int xBins = 400,
    int yBins = 400,
    double xMin = -4.3,
    double xMax = -3.9,
    double yMin = 4.3,
    double yMax = 4.7
) {
    TFile* f = TFile::Open(fileName);
    if (!f || f->IsZombie()) {
        cout << "Failed to open file: " << fileName << endl;
        return 1;
    }
    
    TTree* tree = (TTree*)f->Get(treeName);
    if (!tree) {
        cout << "Failed to get tree: " << treeName << endl;
        f->Close();
        return 1;
    }

    int totalEvents = tree->GetEntries();
    if (endEventNumber < 0 || endEventNumber >= totalEvents) {
        cout << "Invalid event number. The tree contains " << totalEvents << " events." << endl;
        f->Close();
        return 1;
    }
    
    // 创建用于指定事件范围的条件字符串
    stringstream eventSelectStr;
    if (endEventNumber == 0) {
        // 只要事件0
        eventSelectStr << "Entry$==0";
    } else {
        // 所有从0到endEventNumber的事件
        eventSelectStr << "Entry$>=" << 0 << " && Entry$<=" << endEventNumber;
    }
    string eventSelect = eventSelectStr.str();
    
    // 创建绘图命令
    stringstream drawStr;
    drawStr << "OP_pos_final_y:OP_pos_final_x>>h2(" 
            << xBins << "," << xMin << "," << xMax << ","
            << yBins << "," << yMin << "," << yMax << ")";
    string drawCondition = drawStr.str();
    
    // 完整的选择条件
    string selectionCondition = eventSelect + " && OP_pos_final_z>80 && OP_isCoreC";
    
    cout << "Plotting events 0 to " << endEventNumber << " from tree " << treeName << endl;
    cout << "Using bins: " << xBins << "x" << yBins << endl;
    cout << "X range: [" << xMin << ", " << xMax << "]" << endl;
    cout << "Y range: [" << yMin << ", " << yMax << "]" << endl;
    cout << "Selection condition: " << selectionCondition << endl;

    TCanvas* c = new TCanvas("c1", "Histogram", 800, 600);
    
    // 绘制直方图
    tree->Draw(drawCondition.c_str(), selectionCondition.c_str(), "colz");

    TH2F* h2 = (TH2F*)gDirectory->Get("h2");
    if (!h2) {
        cout << "Failed to get histogram. No photons might be selected with the given conditions." << endl;
        delete c;
        f->Close();
        return 1;
    }

    // 设置标题，包含事件范围信息
    stringstream titleStr;
    if (endEventNumber == 0) {
        titleStr << "Photon Distribution - Event 0 (" << xBins << "x" << yBins << " bins)";
    } else {
        titleStr << "Photon Distribution - Events 0 to " << endEventNumber << " (" << xBins << "x" << yBins << " bins)";
    }
    h2->SetTitle(titleStr.str().c_str());
    h2->SetXTitle("x [cm]");
    h2->SetYTitle("y [cm]");

    gStyle->SetOptStat(0);
    gStyle->SetStatX(0.1);
    gStyle->SetStatY(0.9);

    h2->Draw("colz");

    // 找出最大值bin的信息
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

    // 输出最大值bin的信息
    cout << "Information of the maximum bin:" << endl;
    cout << "Bin number: (" << binX << ", " << binY << ")" << endl;
    cout << "Bin position: x = [" << xLow << ", " << xHigh << "], center = " << xCenter << endl;
    cout << "Bin position: y = [" << yLow << ", " << yHigh << "], center = " << yCenter << endl;
    cout << "Count: " << maxCount << endl;

    // 计算直方图的总条目数
    double totalEntries = h2->GetEntries();
    cout << "Total entries in histogram: " << totalEntries << endl;

    // 保存图像文件，文件名包含事件范围信息
    stringstream outFileStr;
    if (endEventNumber == 0) {
        outFileStr << "2DHistogram_" << xBins << "x" << yBins << "_event0.png";
    } else {
        outFileStr << "2DHistogram_" << xBins << "x" << yBins << "_events0to" << endEventNumber << ".png";
    }
    c->SaveAs(outFileStr.str().c_str());

    delete c;
    f->Close();
    delete f;

    return 0;
}
