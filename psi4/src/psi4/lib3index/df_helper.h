/*
 * @BEGIN LICENSE
 *
 * Psi4: an open-source quantum chemistry software package
 *
 * Copyright (c) 2007-2017 The Psi4 Developers.
 *
 * The copyrights for code used from other parties are included in
 * the corresponding files.
 *
 * This file is part of Psi4.
 *
 * Psi4 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Psi4 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with Psi4; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @END LICENSE
 */

#ifndef three_index_df_helper
#define three_index_df_helper


#include "psi4/psi4-dec.h"
#include <psi4/libmints/typedefs.h>

#include <map>
#include <list>
#include <vector>
#include <tuple>
#include <string>

namespace psi {

class BasisSet;
class Options;
class Matrix;
class ERISieve;
class TwoBodyAOInt;

namespace df_helper{

class DF_Helper {

public:

    DF_Helper(std::shared_ptr<BasisSet> primary, std::shared_ptr<BasisSet> aux);
    ~DF_Helper();

    // user options, must set before calling build()---------------------------------
    // memory in doubles
    void set_memory(size_t mem) { memory_ = mem;}
    size_t get_memory() { return memory_; }

    // workflow method (store or direct)
    void set_method(std::string met) { method_ = met;}
    std::string get_method() { return method_; }

    // number of threads
    void set_nthreads(size_t threads) { nthreads_ = threads;}
    size_t get_nthreads() { return nthreads_; }

    // schwarz cutoff
    void set_schwarz_cutoff(double cutoff) { cutoff_ = cutoff;}
    double get_schwarz_cutoff() { return cutoff_; }

    // Do you want it outrageously fast?
    void set_on_core(bool on) {core_ = on;}
    bool get_on_core() { return core_; }

    // metric power (defaults to -0.5)
    void set_metric_pow(double pow) { mpower_ = pow;}
    double get_metric_pow() { return mpower_;}

    void hold_met(bool hold) {hold_met_ = hold;}
    bool get_hold_met() { return hold_met_;}

    // tell me what the worst MO index size is!
    void set_MO_hint(size_t wMO) {wMO_ = wMO;}
    size_t get_MO_hint() { return wMO_;}
    // user options, must set before build---------------------------------

    // Initialize the object
    void initialize();

    // Add transformation space with key
    void add_space(std::string key, SharedMatrix M);

    // add transformation with name using two space keys
    void add_transformation(std::string name, std::string key1, std::string key2, std::string order = "Qpq");

    // invoke transformations
    void transform();

    // obtain transformed integral, however you want
    void fill_tensor(std::string name, SharedMatrix M);
    void fill_tensor(std::string name, SharedMatrix M, std::pair<size_t, size_t> a1);
    void fill_tensor(std::string name, SharedMatrix M, std::pair<size_t, size_t> a1,
      std::pair<size_t, size_t> a2);
    void fill_tensor(std::string name, SharedMatrix M, std::pair<size_t, size_t> a1,
      std::pair<size_t, size_t> a2, std::pair<size_t, size_t> a3);

    // with SharedMatrix returns
    SharedMatrix get_tensor(std::string name);
    SharedMatrix get_tensor(std::string name, std::pair<size_t, size_t> a1);
    SharedMatrix get_tensor(std::string name, std::pair<size_t, size_t> a1,
      std::pair<size_t, size_t> a2);
    SharedMatrix get_tensor(std::string name, std::pair<size_t, size_t> a1,
      std::pair<size_t, size_t> a2, std::pair<size_t, size_t> a3);

    // tranpose a tensor
    void transpose(std::string name, std::tuple<size_t, size_t, size_t> order);
    
    // clear spaces/transformations
    void clear();

    // get sizes, shapes
    size_t get_space_size(std::string key);
    size_t get_tensor_size(std::string key);
    std::tuple<size_t, size_t, size_t> get_tensor_shape(std::string key);

    // => JK <=
    void build_JK(std::vector<SharedMatrix> Cleft, std::vector<SharedMatrix> Cright, 
        std::vector<SharedMatrix> J, std::vector<SharedMatrix> K); 

protected:

    // basis sets
    std::shared_ptr<BasisSet> primary_;
    std::shared_ptr<BasisSet> aux_;
    size_t nao_;
    size_t naux_;

    // => Defaults <=
    // user must tweak these before build

    // memory in doubles
    size_t memory_ = 256000000;

    // method directive
    std::string method_ = "STORE";
    bool direct_;
    bool AO_core_;

    // threading
    size_t nthreads_ = 1;

    // schwarz cutoff
    double cutoff_ = 1e-12;

    // metric power
    double mpower_ = -0.5;
    bool hold_met_ = false;

    // => Internal holders <=
    bool built = false;    

    // if you give me enough memory, i'll turn on the boosters
    bool core_ = 0;
    std::vector<double> Ppq_;
    std::map<double, SharedMatrix> metrics_;

    // => AO building machinery <=
    void prepare_blocking();
    void prepare_AO();
    void prepare_AO_core();

    // indexing for screened AO integrals (so useful)
    std::vector<size_t> small_skips_;
    std::vector<size_t> big_skips_;

    // shell blocking sizes and steps
    size_t pshells_;
    size_t Qshells_;
    std::pair<size_t, size_t> plargest_;
    std::pair<size_t, size_t> Qlargest_;
    std::vector<std::pair<size_t, size_t>> psteps_;
    std::vector<std::pair<size_t, size_t>> Qsteps_;

    // shell size vectors
    std::vector<size_t> pshell_aggs_;
    std::vector<size_t> Qshell_aggs_;

    // general blocking determination
    size_t wMO_=0;
    std::pair<size_t, size_t> shell_blocks(const size_t mem, const size_t guess,
      bool op, const size_t shell_tots, std::vector<std::pair<size_t, size_t>>& b);

    // direct AO building (Q blocking)
    void compute_AO_Q(const size_t start, const size_t stop, double* Mp, std::vector<std::shared_ptr<TwoBodyAOInt>> eri);

    // store AO building (p blocking)
    void compute_AO_p(const size_t start, const size_t stop, double* Mp, std::vector<std::shared_ptr<TwoBodyAOInt>> eri);

    // store AO grabs
    void grab_AO(const size_t start, const size_t stop, double* Mp);

    // Schwarz Screening
    std::vector<size_t> schwarz_fun_mask_;
    std::vector<size_t> schwarz_shell_mask_;
    std::vector<size_t> schwarz_fun_count_;
    void prepare_sparsity();
    void print_masks();

    // metric files and setup
    std::vector<std::pair<double, std::string>> metric_keys_;
    void prepare_metric();
    void prepare_metric_core();
    double* metric_prep_core(double pow);
    std::string return_metfile(double pow);
    std::string compute_metric(double pow);

    // Metric contraction
    void contract_metric(std::string file, double* metp, double* Mp, double* Fp, const size_t tots);
    void contract_metric_core(std::string file);
    void contract_metric_AO(double* Mp);
    void contract_metric_AO_core(double* Qpq, double* metp);

    // spaces and transformation maps
    std::map<std::string, std::tuple<SharedMatrix, size_t>> spaces_;
    std::map<std::string, std::tuple<std::string, std::string, std::string>> transf_;
    std::map<std::string, std::vector<double>> transf_core_;

    // transformation machinery
    void transform_core();
    void transform_disk();
    std::vector<std::pair<std::string, size_t>> sorted_spaces_;
    std::vector<std::string> order_;
    std::vector<std::string> bspace_;
    std::vector<size_t> strides_;
    
    bool once_=false;
    std::pair<size_t, size_t> info_;
    bool ordered_=0;
    std::pair<size_t, size_t> identify_order();
    void print_order();

    // file stream maintence
    struct stream{
        FILE* fp;
        std::string op;};
    std::map<std::string, stream> file_status_;

    //std::map<std::string, std::tuple<FILE* , std::string>> file_status_;
    FILE* stream_check(std::string filename, std::string op);

    // file i/o machinery
    void put_tensor(std::string file, double* b, std::pair<size_t, size_t> a1,
      std::pair<size_t, size_t> a2, std::pair<size_t, size_t> a3, std::string op);
    void put_tensor(std::string file, double* b, const size_t start1,
      const size_t stop1, const size_t start2, const size_t stop2, std::string op);
    void get_tensor_(std::string file, double* b, std::pair<size_t, size_t> a1,
      std::pair<size_t, size_t> a2, std::pair<size_t, size_t> a3);
    void get_tensor_(std::string file, double* b,  const size_t start1,
      const size_t stop1, const size_t start2, const size_t stop2);

    // AO file i/o machinery
    void put_tensor_AO(std::string file, double* Mp, size_t size, size_t start, std::string op);
    void get_tensor_AO(std::string file, double* Mp, size_t size, size_t start);

    // file accesses
    std::map<std::string, std::tuple<std::string, std::string>> files_;
    std::map<std::string, std::tuple<size_t, size_t, size_t>> sizes_;
    std::map<std::string, std::tuple<size_t, size_t, size_t>> tsizes_;
    std::map<std::string, std::string> AO_files_;
    std::vector<size_t> AO_file_sizes_;
    std::vector<std::string> AO_names_;
    void filename_maker(std::string name, size_t a0, size_t a1, size_t a2);
    void AO_filename_maker(size_t i);
    void check_transformation_name(std::string);
    void check_transformation_tuple(std::string name, std::pair<size_t, size_t> t0, 
        std::pair<size_t, size_t> t1, std::pair<size_t, size_t> t2);
    void check_transformation_matrix(std::string name, SharedMatrix M, std::pair<size_t, size_t> t0,
        std::pair<size_t, size_t> t1, std::pair<size_t, size_t> t2);

    // tranpose a tensor
    void transpose_core(std::string name, std::tuple<size_t, size_t, size_t> order);
    void transpose_disk(std::string name, std::tuple<size_t, size_t, size_t> order);

    // => JK <=
    void JK_core(std::vector<SharedMatrix> Cleft, std::vector<SharedMatrix> Cright, 
        std::vector<SharedMatrix> J, std::vector<SharedMatrix> K); 
    void JK_disk(std::vector<SharedMatrix> Cleft, std::vector<SharedMatrix> Cright, 
        std::vector<SharedMatrix> J, std::vector<SharedMatrix> K); 
    void compute_D(std::vector<SharedMatrix>& D, std::vector<SharedMatrix> Cleft, std::vector<SharedMatrix> Cright);
    void compute_J(std::vector<SharedMatrix> D, std::vector<SharedMatrix> J, double* Mp, 
        double* T1p, double* T2p, double** D_buffers, size_t bcount, size_t block_size);
    void compute_K(std::vector<SharedMatrix> Cleft, 
        std::vector<SharedMatrix> Cright, std::vector<SharedMatrix> K, double* Tp, double* Jtmp, 
        double* Mp, size_t bcount, size_t block_size, double** C_buffers);

}; // End DF Helper class

}}//end df_helper/psi4 namespace
#endif
