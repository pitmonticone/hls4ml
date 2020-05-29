//
//    rfnoc-hls-neuralnet: Vivado HLS code for neural-net building blocks
//
//    Copyright (C) 2017 EJ Kreinar
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "firmware/myproject.h"


int main(int argc, char **argv)
{
  //load input data from text file
  std::ifstream fin("tb_data/tb_input_node_features.dat");
  std::ifstream fie("tb_data/tb_input_edge_features.dat");
  std::ifstream fre("tb_data/tb_receivers.dat");
  std::ifstream fse("tb_data/tb_senders.dat");
  //load predictions from text file
  std::ifstream fpr("tb_data/tb_output_edge_predictions.dat");

#ifdef RTL_SIM
  std::string RESULTS_LOG = "tb_data/rtl_cosim_results.log";
#else
  std::string RESULTS_LOG = "tb_data/csim_results.log";
#endif
  std::ofstream fout(RESULTS_LOG);

  std::string iline;
  std::string ieline;
  std::string reline;
  std::string seline;
  std::string pline;
  int e = 0;

  while ( std::getline(fin,iline) && std::getline(fie,ieline) && std::getline(fre,reline) && std::getline(fse,seline) && std::getline (fpr,pline) ) {
    std::cout << "Processing input " << e << std::endl;
    
    char* cstr=const_cast<char*>(iline.c_str());
    char* current;
    std::vector<float> in;
    current=strtok(cstr," ");
    while(current!=NULL) {
      in.push_back(atof(current));
      current=strtok(NULL," ");
    }
    
    cstr=const_cast<char*>(ieline.c_str());
    std::vector<float> ie;
    current=strtok(cstr," ");
    while(current!=NULL) {
      ie.push_back(atof(current));
      current=strtok(NULL," ");
    }

    cstr=const_cast<char*>(reline.c_str());
    std::vector<int> re;
    current=strtok(cstr," ");
    while(current!=NULL) {
      re.push_back(atof(current));
      current=strtok(NULL," ");
    }
    
    cstr=const_cast<char*>(seline.c_str());
    std::vector<int> se;
    current=strtok(cstr," ");
    while(current!=NULL) {
      se.push_back(atof(current));
      current=strtok(NULL," ");
    }
    
    cstr=const_cast<char*>(pline.c_str());
    std::vector<float> pr;
    current=strtok(cstr," ");
    while(current!=NULL) {
      pr.push_back(atof(current));
      current=strtok(NULL," ");
    }
    
    //hls-fpga-machine-learning insert data
    input_t X_str[N_NODES][N_NODE_FEATURES];
    for(int i = 0; i < N_NODES; i++) {
      for(int j = 0; j < N_NODE_FEATURES; j++) {
	X_str[i][j] = in[j+N_NODE_FEATURES*i];
      }
    }

    input_t Y_str[N_EDGES][N_EDGE_FEATURES];
    for(int i = 0; i < N_EDGES; i++) {
      for(int j = 0; j < N_EDGE_FEATURES; j++) {
	Y_str[i][j] = in[j+N_EDGE_FEATURES*i];
      }
    }

    adjacency_t Rr_str[N_EDGES][1];
    for(int i = 0; i < N_EDGES; i++) {
      Rr_str[i][0] = re[i];
    }

    adjacency_t Rs_str[N_EDGES][1];
    for(int i = 0; i < N_EDGES; i++) {
      Rs_str[i][0] = se[i];
    }

    result_t e_str[N_EDGES][1];
    for(int i=0; i<N_EDGES; i++){
      e_str[i][0]=0;
    }

    unsigned short size_in, size_out;
    myproject(X_str, Y_str, Rr_str, Rs_str, e_str, size_in, size_out);
    
    //hls-fpga-machine-learning insert tb-output
    for(int i = 0; i < N_EDGES; i++) {
      fout << e_str[i][0] << " ";
    }
    fout << std::endl;
    
    std::cout << "Predictions" << std::endl;
    //hls-fpga-machine-learning insert predictions
    for(int i = 0; i < N_EDGES; i++) {
      std::cout << pr[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Quantized predictions" << std::endl;
    //hls-fpga-machine-learning insert quantized
    for(int i = 0; i < N_EDGES; i++) {
      std::cout << e_str[i][0] << " ";
      }
      std::cout << std::endl;
  }
  fin.close();
  fie.close();
  fre.close();
  fse.close();
  fpr.close();  
  fout.close();
  std::cout << "INFO: Saved inference results to file: " << RESULTS_LOG << std::endl;
  
  return 0;
}


