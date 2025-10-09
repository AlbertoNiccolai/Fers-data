#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <cmath>
#include <sstream>
#include <cstdint>  
using namespace std;

void StaircasePlot(TString fileinput){
    fstream file;
    file.open(fileinput, ios::in);
    if (!file.is_open()) {
        cerr << "Error while opening file " << fileinput << endl;
        return;
    }

    //Parameters
    int board;
    uint64_t Tmask;
    int Threshold;
    double T_Or, Q_Or;
    double ch[64];
    int start, step, Nstep;
    float time;

    string line;

    file>>line>>board; 
    file>>line>>hex>>Tmask;
    file>>line>>dec>>start;
    file>>line>>step;
    file>>line>>Nstep;
    file>>line>>time;
    cout<<"board = "<<board<<endl;
    cout<<"Tmask = 0x"<<hex<<Tmask<<endl;
    cout<<"start = "<<dec<<start<<endl;
    cout<<"stop = "<<(start + step * Nstep)<<endl;
    cout<<"step = "<<step<<endl;
    cout<<"time = "<<time<<endl;

    TH1F *hist[64] = {nullptr};
    TCanvas *canvas[64] = {nullptr};
    for(int i=0; i<64;i++){
        if((Tmask>>i)&1){
            hist[i] = new TH1F(Form("histch%d", i), Form("histch%d", i), Nstep, start, start + step * Nstep);
            canvas[i] = new TCanvas(Form("Staircase_%d", i));
        }
    }
    //TCanvas *cTOr = new TCanvas("T_Or");
    //TCanvas *cQOr = new TCanvas("Q_Or");
    TH1F *histTOr = new TH1F("histTOr", "histTOr", Nstep, start, start + step * Nstep);
    TH1F *histQOr = new TH1F("histQOr", "histQOr", Nstep, start, start + step * Nstep);

    // Skip 3 lines
    for (int i = 0; i < 3; ++i) {
        getline(file, line);
    }

    vector<vector<double>> data;
    double val;
    int j = 0;
    int bin = Nstep;

    while(getline(file,line)){
        if(file.eof()) break;
        //cout<<line<<endl;
        istringstream ss(line);
        vector<double> row;
        while(ss>>val){
            row.push_back(val);
        }
        data.push_back(row);
        j=1;
        for(int i=0; i<64;i++){
            if((Tmask>>i)&1){
                hist[i]->SetBinContent(bin, row[j]);
                //cout<<row[j]<<"  ";
                j++;
            }
        }
        //cout<<endl;
        histTOr->Fill(row[j+1]);
        histQOr->Fill(row[j+2]);
        bin--;
    }

    file.close();

    for (int i = 0; i < 64; ++i) {
        if((Tmask>>i)&1){
            canvas[i]->cd();
            canvas[i]->SetLogy(1);
            hist[i]->GetXaxis()->SetTitle("Threshold [DAC counts]");
            hist[i]->GetYaxis()->SetTitle("Rate [Hz]");
            hist[i]->Draw();
        }
    }
    /*cTOr->cd();
    cTOr->SetLogy(1);
    histTOr->GetXaxis()->SetTitle("Threshold [DAC counts]");
    histTOr->GetYaxis()->SetTitle("Rate [Hz]");
    histTOr->Draw();

    cQOr->cd();
    cQOr->SetLogy(1);
    histQOr->GetXaxis()->SetTitle("Threshold [DAC counts]");
    histQOr->GetYaxis()->SetTitle("Rate [Hz]");
    histQOr->Draw();*/
}


void DoubleStaircase(TString fileinput1, TString fileinput2){

    fstream file1;
    file1.open(fileinput1, ios::in);
    if (!file1.is_open()) {
        cerr << "Error while opening file " << fileinput1 << endl;
        return;
    }

    //Parameters
    int board;
    uint64_t Tmask1;
    int Threshold;
    double T_Or, Q_Or;
    double ch[64];
    int start, step, Nstep;
    float time;

    string line;

    file1>>line>>board; 
    file1>>line>>hex>>Tmask1;
    file1>>line>>dec>>start;
    file1>>line>>step;
    file1>>line>>Nstep;
    file1>>line>>time;
    cout<<"board = "<<board<<endl;
    cout<<"Tmask = 0x"<<hex<<Tmask1<<endl;
    cout<<"start = "<<dec<<start<<endl;
    cout<<"stop = "<<(start + step * Nstep)<<endl;
    cout<<"step = "<<step<<endl;
    cout<<"time = "<<time<<endl;

    TH1F *hist1[64] = {nullptr};
    TCanvas *canvas[64] = {nullptr};
    for(int i=0; i<64;i++){
        if((Tmask1>>i)&1){
            hist1[i] = new TH1F(Form("hist1ch%d", i), Form("hist1ch%d", i), Nstep, start, start + step * Nstep);
            canvas[i] = new TCanvas(Form("Staircase_%d", i));
        }
    }

    // Skip 3 lines
    for (int i = 0; i < 3; ++i) {
        getline(file1, line);
    }

    vector<vector<double>> data;
    double val;
    int j = 0;
    int bin = Nstep;

    while(getline(file1,line)){
        if(file1.eof()) break;
        //cout<<line<<endl;
        istringstream ss(line);
        vector<double> row;
        while(ss>>val){
            row.push_back(val);
        }
        data.push_back(row);
        j=1;
        for(int i=0; i<64;i++){
            if((Tmask1>>i)&1){
                hist1[i]->SetBinContent(bin, row[j]);
                //cout<<row[j]<<"  ";
                j++;
            }
        }
        //cout<<endl;
        bin--;
    }

    file1.close();

    uint64_t Tmask2;
    fstream file2;
    file2.open(fileinput2, ios::in);
    if (!file2.is_open()) {
        cerr << "Error while opening file " << fileinput2 << endl;
        return;
    }


    file2>>line>>board; cout<<line<<endl;
    file2>>line>>hex>>Tmask2;
    file2>>line>>dec>>start;
    file2>>line>>step;
    file2>>line>>Nstep;
    file2>>line>>time;
    cout<<"board = "<<board<<endl;
    cout<<"Tmask = 0x"<<hex<<Tmask2<<endl;
    cout<<"start = "<<dec<<start<<endl;
    cout<<"stop = "<<(start + step * Nstep)<<endl;
    cout<<"step = "<<step<<endl;
    cout<<"time = "<<time<<endl;

    TH1F *hist2[64] = {nullptr};
    for(int i=0; i<64;i++){
        if((Tmask2>>i)&1){
            hist2[i] = new TH1F(Form("hist2ch%d", i), Form("hist2ch%d", i), Nstep, start, start + step * Nstep);
        }
    }

    // Skip 3 lines
    for (int i = 0; i < 3; ++i) {
        getline(file2, line);
    }
    bin = Nstep;

    while(getline(file2,line)){
        if(file2.eof()) break;
        //cout<<line<<endl;
        istringstream ss(line);
        vector<double> row;
        while(ss>>val){
            row.push_back(val);
        }
        data.push_back(row);
        j=1;
        for(int i=0; i<64;i++){
            if((Tmask2>>i)&1){
                hist2[i]->SetBinContent(bin, row[j]);
                //cout<<row[j]<<"  ";
                j++;
            }
        }
        //cout<<endl;
        bin--;
    }

    file2.close();
    double ymax;

    for (int i = 0; i < 64; ++i) {
        if((Tmask1>>i)&(Tmask2>>i)&1){
            canvas[i]->cd();
            canvas[i]->SetLogy(1);
            ymax = std::max(hist1[i]->GetMaximum(), hist2[i]->GetMaximum());
            hist1[i]->SetMaximum(ymax*1.2);
            hist1[i]->GetXaxis()->SetTitle("Threshold [DAC counts]");
            hist1[i]->GetYaxis()->SetTitle("Rate [Hz]");
            hist1[i]->SetLineColor(kRed);
            hist1[i]->SetLineWidth(2);
            hist1[i]->SetFillStyle(1001);
            //hist1[i]->SetFillColorAlpha(kOrange, 0.5);
            hist1[i]->SetFillColor(kOrange);
            hist1[i]->Draw();

            hist2[i]->GetXaxis()->SetTitle("Threshold [DAC counts]");
            hist2[i]->GetYaxis()->SetTitle("Rate [Hz]");
            hist2[i]->SetLineColor(kBlue);
            hist2[i]->SetLineWidth(2);
            hist2[i]->SetFillStyle(3001);
            hist2[i]->SetFillColorAlpha(kCyan, 0.5);
            hist2[i]->Draw("same");
        }
    }
}