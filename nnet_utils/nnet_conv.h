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

#ifndef NNET_CONV_H_
#define NNET_CONV_H_

#include "nnet_common.h"
#include <cstdlib>

namespace nnet {

struct conv_config
{
    // Internal data type definitions                                                                                      
    typedef float bias_t;
    typedef float weight_t;
    typedef float accum_t;

    // Convolutional parameters
    static const unsigned pad_left = 4;
    static const unsigned pad_right = 5;
    static const unsigned y_in = 128;
    static const unsigned n_chan = 9;
    static const unsigned y_filt = 10;
    static const unsigned n_filt = 4;
    static const unsigned stride = 1;
    static const unsigned y_out = 128; 
  
    static const unsigned reuse_factor = 1;
    static const bool store_weights_in_bram = false;
    static const unsigned n_zeros = 0; // not used yet
};


//Computes multiplier limit
//This function should not be synthesized into firmware
template<typename CONFIG_T>
int compute_multiplier_limit(
    typename CONFIG_T::weight_t  weights[CONFIG_T::y_filt * CONFIG_T::n_chan * CONFIG_T::n_filt]
)
{
    int n_mult = 0;
    for(int ii = 0; ii < CONFIG_T::y_out; ii++) {
        for(int ff = 0; ff < CONFIG_T::n_filt; ff++){
	    for(int cc = 0; cc < CONFIG_T::n_chan; cc++){
                for(int jj = 0; jj < CONFIG_T::y_filt; jj++){
                    
                    int index_weight = jj*CONFIG_T::n_chan*CONFIG_T::n_filt + cc*CONFIG_T::n_filt + ff;
                    
                    if((ii*CONFIG_T::stride+jj) < CONFIG_T::pad_left || (ii*CONFIG_T::stride+jj) >= (CONFIG_T::pad_left + CONFIG_T::y_in)){
			//padded -- do nothing
			continue;
                    }
                    else {
			//need to tune this cut?
                        if( weights[index_weight] > 1e-20 || weights[index_weight] < -1e-20 ){
			    n_mult++;
			}//end if nonzero weight
                    }//end not padding
                }//end loop accross filter
	    }//end channel loop
	}//end filter loop
    }//end output loop

    return ceil( float(n_mult) / float(CONFIG_T::reuse_factor) );
   
}//end compute_n_mult


template<class data_T, class res_T, typename CONFIG_T>
void conv_1d(
	     data_T    data[CONFIG_T::y_in][CONFIG_T::n_chan],
	     res_T     res[CONFIG_T::y_out][CONFIG_T::n_filt],
	     typename CONFIG_T::weight_t  weights[CONFIG_T::y_filt * CONFIG_T::n_chan * CONFIG_T::n_filt],
	     typename CONFIG_T::bias_t    biases[CONFIG_T::n_filt])
{

    //typename CONFIG_T::accum_t mult[CONFIG_T::y_out * CONFIG_T::n_filt * CONFIG_T::n_chan * CONFIG_T::y_filt];
    //#pragma HLS ARRAY_PARTITION variable=mult complete dim=0

    typename CONFIG_T::accum_t acc[CONFIG_T::y_out][CONFIG_T::n_filt];
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=0

    // Use a function_instantiate in case it helps to explicitly optimize unchanging weights/biases 
    #pragma HLS function_instantiate variable=weights,biases

    // Limit multipliers to control parallelization
    //const int multiplier_limit = compute_multiplier_limit<CONFIG_T>(weights);
    const int multiplier_limit = CONFIG_T::n_filt*CONFIG_T::y_out;
    //#pragma HLS ALLOCATION instances=mul limit=multiplier_limit operation
    
    // Parallel mode
    //#pragma HLS PIPELINE
    #pragma HLS ARRAY_PARTITION variable=biases complete dim=0
    //consider ARRAY_RESHAPE for data in and/or out

    #pragma HLS DEPENDENCE variable=acc,weights,biases inter false

    // core functionality
    //int rufactor=CONFIG_T::reuse_factor;
    //int rufactor=CONFIG_T::y_out*CONFIG_T::n_filt;
    int rufactor=CONFIG_T::n_chan*CONFIG_T::y_filt;
    // a tmp mult for each reuse loop iteration
    typename CONFIG_T::accum_t mult[multiplier_limit];
    #pragma HLS ARRAY_PARTITION variable=mult complete
    #pragma HLS DEPENDENCE variable=mult inter false


    const int ADD_LAT = DIV_ROUNDUP(multiplier_limit,CONFIG_T::n_filt*CONFIG_T::y_out);//should equal 1 for this case
    ReuseLoop: for (int ir = 0; ir < rufactor; ir++){
      
      #pragma HLS PIPELINE II=1 rewind
      ///////// --------------------------------------

        MultLoop: 
        for (int im = 0; im < multiplier_limit; im++){

        }

        // special loop for accumulation
        typename CONFIG_T::accum_t acc_lat[CONFIG_T::n_filt*CONFIG_T::y_out][ADD_LAT];
        #pragma HLS ARRAY_PARTITION variable=acc_lat complete dim=0
        #pragma HLS DEPENDENCE variable=acc_lat inter false

        AddLatencyInit: 
        for (int ii = 0; ii < CONFIG_T::n_out; ii++){
	  for (int ij= 0; ij < ADD_LAT; ij++){
            #pragma UNROLL
	    acc_lat[ii][ij] = 0;
	  }
        }
        
        AccumLoop:
	for (int io = 0; io < CONFIG_T::n_out; io++){
          #pragma HLS UNROLL
	  for (int ia = 0; ia < ADD_LAT; ia++){
            #pragma HLS UNROLL
	    
             acc_lat[out_index_acc][ia] += mult[mult_index_acc];

	  }
	}

        FullAccum: 
	  for (int ii = 0; ii < CONFIG_T::n_out; ii++){
	    for (int ij= 0; ij < ADD_LAT; ij++){
              #pragma HLS UNROLL
	      acc[ii] += acc_lat[ii][ij];
	    }
          }
   
    }//reuse

    /*
    // Convolve, saving all multiplication results to accumulate later
    ConvOut: for(int ii = 0; ii < CONFIG_T::y_out; ii++) {
        ConvFilt: for(int ff = 0; ff < CONFIG_T::n_filt; ff++){
            ConvChan: for(int cc = 0; cc < CONFIG_T::n_chan; cc++){
                ConvMult: for(int jj = 0; jj < CONFIG_T::y_filt; jj++){
                    
                    int index_mult   = ii*CONFIG_T::n_filt*CONFIG_T::n_chan*CONFIG_T::y_filt + ff*CONFIG_T::n_chan*CONFIG_T::y_filt + cc*CONFIG_T::y_filt + jj;
                    int index_weight = jj*CONFIG_T::n_chan*CONFIG_T::n_filt + cc*CONFIG_T::n_filt + ff;
                    
                    if((ii*CONFIG_T::stride+jj) < CONFIG_T::pad_left || (ii*CONFIG_T::stride+jj) >= (CONFIG_T::pad_left + CONFIG_T::y_in)){
                        mult[index_mult] = 0;
                    }
                    else {
                        mult[index_mult] = data[ii*CONFIG_T::stride+jj-CONFIG_T::pad_left][cc] * weights[index_weight];
                    }
                }
	    	}//end channel loop
		}//end filter loop
    }//end output loop


    // Initialize accumulator with input biases
    for(int ii = 0; ii < CONFIG_T::y_out; ii++) {
		for(int ff = 0; ff < CONFIG_T::n_filt; ff++) {
	    	acc[ii][ff]=biases[ff];
		}
    }

    
    // Accumulate multiplication result
    AccumOut: for(int ii = 0; ii < CONFIG_T::y_out; ii++) {
        AccumFilt: for(int ff = 0; ff < CONFIG_T::n_filt; ff++) {
			//Do "dot product" sum within filter and sum over channels
            AccumChan: for(int cc = 0; cc < CONFIG_T::n_chan; cc++){
			    AccumDot: for(int jj = 0; jj < CONFIG_T::y_filt; jj++){
                    int index_mult = ii*CONFIG_T::n_filt*CONFIG_T::n_chan*CONFIG_T::y_filt + ff*CONFIG_T::n_chan*CONFIG_T::y_filt + cc*CONFIG_T::y_filt + jj;
		    		acc[ii][ff] += mult[index_mult];
                }//end dot product loop
	    	}//end channel loop
		}//end filter loop
    }//end output loop
    */
    
     // Cast to "res_t" type 
    for(int ii = 0; ii < CONFIG_T::y_out; ii++) {
      for(int ff = 0; ff < CONFIG_T::n_filt; ff++) {
	res[ii][ff] = (res_T)(acc[ii][ff]);
      }
    }

}//conv1d


template<class data_T, int NROWS, int NCOLS>
    void flatten(
        data_T    data[NROWS][NCOLS], 
	data_T     res[NROWS*NCOLS])
{

    //Initialize
    //for(int i=0; i<NROWS*NCOLS; i++){
    //    res[i]=0;
    //}

    for(int r=0; r<NROWS; r++){
        for(int c=0; c<NCOLS; c++){
            res[r*NCOLS+c] = data[r][c];
        }
    }
}


template<class data_T, int NROWS, int NCOLS>
    void unflatten(
        data_T    data[NROWS*NCOLS], 
	data_T     res[NROWS][NCOLS])
{
    for(int r=0; r<NROWS; r++){
        for(int c=0; c<NCOLS; c++){
             res[r][c] = data[r*NCOLS+c];
        }
    }
}




}//end namespace

#endif
