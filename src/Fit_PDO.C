#include <iostream>

#include "include/setstyle.hh"
#include "include/fit_functions.hh"
#include "include/VMM_data.hh"

using namespace std;

int main(int argc, char* argv[]){
  setstyle();

  char inputFileName[400];
  char outputFileName[400];

  if ( argc < 2 ){
    cout << "Error at Input: please specify an input .root file ";
    cout << "and an (optional) output filename" << endl;
    cout << "Example:   ./Fit_PDO input_file.root" << endl;
    cout << "Example:   ./Fit_PDO input_file.root -o output_file.root" << endl;
    return 1;
  }

  sscanf(argv[1],"%s", inputFileName);
  bool user_output = false;
  for (int i=0;i<argc;i++){
    if (strncmp(argv[i],"-o",2)==0){
      sscanf(argv[i+1],"%s", outputFileName);
      user_output = true;
    }
  }

  string output_name;
  if(!user_output){
    string input_name = string(inputFileName);
    // strip path from input name
    while(input_name.find("/") != string::npos)
      input_name.erase(input_name.begin(),
		       input_name.begin()+
		       input_name.find("/")+1);
    if(input_name.find(".root") != string::npos)
      input_name.erase(input_name.find(".root"),5);
    output_name = input_name+"_PDOfit.root";
    sprintf(outputFileName,"%s.root",inputFileName);
  } else {
    output_name = string(outputFileName);
  }

  TChain* tree = new TChain("calib");
  tree->AddFile(inputFileName);

  VMM_data* base = new VMM_data(tree);

  int N = tree->GetEntries();
  if(N == 0) return 0;
  
  map<pair<int,int>, int> MMFE8VMM_to_index;
  vector<map<int,int> >   vCH_to_index;
  vector<vector<map<int,int> > >  vDAC_to_index;

  vector<vector<map<int, TH1D*> > > vDAC_to_hist;
  
  vector<int>             vMMFE8;               // board ID
  vector<int>             vVMM;                 // VMM number
  vector<vector<int> >    vCH;                  // channel number
  vector<vector<vector<TH1D*> > >  vhist;       // PDO histograms
  vector<vector<vector<int> > > vminpdo;        // min PDOs, per DAC
  vector<vector<vector<int> > > vmaxpdo;        // max PDOs, per DAC
  vector<vector<vector<int> > > vDAC;           // DAC value
  vector<vector<vector<string> > > vlabel;      // label

  int MMFE8;
  int VMM;
  int DAC;
  int CH;

  // find the min, max, range
  for (int i = 0; i < N; i++){
    base->GetEntry(i);

    for (int j = 0; j < base->chip->size(); j++){ 

      for (int k = 0; k < base->channel->at(j).size(); k++){

        if(base->pdo->at(j).at(k) <=  0)
          continue;

        if(base->tdo->at(j).at(k) <=  0)
          continue;


        MMFE8 = base->boardId->at(j);
        VMM   = base->chip->at(j);
        DAC   = base->pulserCounts;
        CH    = base->channel->at(j).at(k);
    
        // add a new MMFE8+VMM entry if
        // is a new MMFE8+VMM combination
        if(MMFE8VMM_to_index.count(pair<int,int>(MMFE8,VMM)) == 0){
          int ind = int(vMMFE8.size());
          MMFE8VMM_to_index[pair<int,int>(MMFE8,VMM)] = ind;
          vMMFE8.push_back(MMFE8);
          vVMM.push_back(VMM);
          vCH.push_back(vector<int>());
          vCH_to_index.push_back(map<int,int>());
          vDAC_to_index.push_back(vector<map<int,int> >());
          vDAC_to_hist.push_back(vector<map<int,TH1D*> >());
          vhist.push_back(vector<vector<TH1D*> >());
          vDAC.push_back(vector<vector<int> >());
          vlabel.push_back(vector<vector<string> >());
          vminpdo.push_back(vector<vector<int> >());
          vmaxpdo.push_back(vector<vector<int> >());
        }
        
        // MMFE8+VMM index
        int index = MMFE8VMM_to_index[pair<int,int>(MMFE8,VMM)];
        
        // add a new channel if is new for
        // this MMFE8+VMM combination
        if(vCH_to_index[index].count(CH) == 0){
          int ind = int(vCH[index].size());
          vCH_to_index[index][CH] = ind;
          vCH[index].push_back(CH);
          vDAC_to_hist[index].push_back(map<int,TH1D*>());
          vDAC_to_index[index].push_back(map<int,int>());
          vhist[index].push_back(vector<TH1D*>());
          vDAC[index].push_back(vector<int>());
          vlabel[index].push_back(vector<string>());
          vminpdo[index].push_back(vector<int>());
          vmaxpdo[index].push_back(vector<int>());
        }
        
        // CH index
        int cindex = vCH_to_index[index][CH];

        // add a new histogram if this DAC
        // combination is new for the MMFE8+VMM+CH combo
        if(vDAC_to_hist[index][cindex].count(DAC) == 0){
          int ind = int(vDAC[index][cindex].size());
          char sDAC[20];
          sprintf(sDAC,"%d",DAC);
          char shist[20];
          sprintf(shist,"%d_%d_%d_%d",MMFE8,VMM,CH,DAC);
          TH1D* hist = new TH1D(("h_"+string(shist)).c_str(),
                                ("h_"+string(shist)).c_str(),
                                4096, -0.5, 4095.5);

          vDAC_to_index[index][cindex][DAC] = ind;
          vDAC_to_hist[index][cindex][DAC] = hist;
          vhist[index][cindex].push_back(hist);
          vDAC[index][cindex].push_back(DAC);
          vminpdo[index][cindex].push_back(999999.);
          vmaxpdo[index][cindex].push_back(0.);
          vlabel[index][cindex].push_back("DAC = "+string(sDAC));
        }

        // DAC index 
        int dindex = vDAC_to_index[index][cindex][DAC];

        if (base->pdo->at(j).at(k) < vminpdo[index][cindex][dindex])
          vminpdo[index][cindex][dindex] = base->pdo->at(j).at(k);
        if (base->pdo->at(j).at(k) > vmaxpdo[index][cindex][dindex])
          if (DAC > 150 || base->pdo->at(j).at(k) < 1023) //1023)
            vmaxpdo[index][cindex][dindex] = base->pdo->at(j).at(k);
      }
    }
  }

//   for (auto ivmm_maps: vDAC_to_index)
//     for (auto ich_maps: ivmm_maps)
//       for (auto& imap: ich_maps)
//         std::cout << imap.first << " has value " << imap.second <<std::endl;

  for (int i = 0; i < N; i++){
    base->GetEntry(i);

    for (int j = 0; j < base->chip->size(); j++){ 

      for (int k = 0; k < base->channel->at(j).size(); k++){

        if(base->pdo->at(j).at(k) <=  0)
          continue;

        if(base->tdo->at(j).at(k) <=  0)
          continue;


        MMFE8 = base->boardId->at(j);
        VMM   = base->chip->at(j);
        DAC   = base->pulserCounts;
        CH    = base->channel->at(j).at(k);
    
        // MMFE8+VMM index
        int index = MMFE8VMM_to_index[pair<int,int>(MMFE8,VMM)];
        
        // CH index
        int cindex = vCH_to_index[index][CH];

        // DAC index
        int dindex = vDAC_to_index[index][cindex][DAC];

//         if (DAC == 560 && VMM == 0 && CH == 23){
//           std::cout << std::endl;
//           std::cout << "MMFE8 " << MMFE8 << ", VMM " << VMM << ", CH " << CH << ", DAC " << DAC << std::endl;
//           std::cout << "index, cindex, dindex: " << index << " " << cindex << " " << dindex << std::endl;
//           std::cout << "(min pdo, max pdo) = (" << vminpdo[index][cindex][dindex] << ", " << vmaxpdo[index][cindex][dindex]  << ")" << std::endl;
//           std::cout << std::endl;
//           }

        int range = vmaxpdo[index][cindex][dindex] - vminpdo[index][cindex][dindex];
        if ( base->pdo->at(j).at(k) > (vmaxpdo[index][cindex][dindex] * 0.5) )
          if ( (base->pdo->at(j).at(k) < 1023) || (DAC > 150.) )
          vDAC_to_hist[index][cindex][DAC]->Fill(base->pdo->at(j).at(k));
      }
    }
  }

  int Nindex = vMMFE8.size();

  TFile* fout = new TFile(output_name.c_str(), "RECREATE");

  // add plots of PDO values for each
  // MMFE8+VMM+CH combo to output file
  fout->mkdir("PDO_plots");
  fout->cd("PDO_plots");
  for(int i = 0; i < Nindex; i++){ 
    char sfold[50];
    sprintf(sfold, "PDO_plots/Board%d_VMM%d", vMMFE8[i], vVMM[i]);
    //sprintf(sfold, "Board%d_VMM%d", vMMFE8[i], vVMM[i]);
    fout->mkdir(sfold);
    fout->cd(sfold);

    int Ncindex = vCH[i].size();
    for(int c = 0; c < Ncindex; c++){
      char stitle[50];
      sprintf(stitle, "Board #%d, VMM #%d , CH #%d", vMMFE8[i], vVMM[i], vCH[i][c]);
      char scan[50];
      sprintf(scan, "c_PDO_Board%d_VMM%d_CH%d", vMMFE8[i], vVMM[i], vCH[i][c]);
      TCanvas* can = Plot_1D(scan, vhist[i][c], "PDO", "Count", stitle, vlabel[i][c]);
      can->Write();
      delete can;
    }
  }
  fout->cd("");

  // write VMM_data tree to outputfile
  //TTree* newtree = tree->CloneTree();
  //fout->cd();
  //newtree->Write();
  //delete newtree;
  delete base;
  delete tree;

  // Output PDO fit tree
  double fit_MMFE8;
  double fit_VMM;
  double fit_CH;
  double fit_DAC;
  double fit_meanPDO;
  double fit_sigmaPDO;
  double fit_meanPDOerr;
  double fit_sigmaPDOerr;
  double fit_chi2;
  double fit_prob;
  
  TTree* fit_tree = new TTree("PDO_fit", "PDO_fit");
  fit_tree->Branch("MMFE8", &fit_MMFE8);
  fit_tree->Branch("VMM", &fit_VMM);
  fit_tree->Branch("CH", &fit_CH);
  fit_tree->Branch("DAC", &fit_DAC);
  fit_tree->Branch("meanPDO", &fit_meanPDO);
  fit_tree->Branch("meanPDOerr", &fit_meanPDOerr);
  fit_tree->Branch("sigmaPDO", &fit_sigmaPDO);
  fit_tree->Branch("sigmaPDOerr", &fit_sigmaPDOerr);
  fit_tree->Branch("chi2", &fit_chi2);
  fit_tree->Branch("prob", &fit_prob);

  // currently, no fit performed extracting PDO values
  for(int i = 0; i < Nindex; i++){
    fit_MMFE8 = vMMFE8[i];
    fit_VMM = vVMM[i];
    int Ncindex = vCH[i].size();
    for(int c = 0; c < Ncindex; c++){
      fit_CH = vCH[i][c];
      int Ndac = vDAC[i][c].size();
      for(int d = 0; d < Ndac; d++){
	fit_DAC = vDAC[i][c][d];
	double I = vhist[i][c][d]->Integral();
	fit_meanPDO = vhist[i][c][d]->GetMean();
	fit_meanPDOerr = max(vhist[i][c][d]->GetRMS()/sqrt(I),1.);
	fit_sigmaPDO = max(vhist[i][c][d]->GetRMS(),1.);
	fit_sigmaPDOerr = max(vhist[i][c][d]->GetRMSError(),1.);
	fit_chi2 = 0.;
	fit_prob = 1.;
	
	fit_tree->Fill();
      }
    }
  }

  // Write fit_tree to output file
  fout->cd("");
  fit_tree->Write();
  fout->Close();
}
