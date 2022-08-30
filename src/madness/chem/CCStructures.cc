/*
 * CCStructures.cc
 *
 *  Created on: Jan 4, 2017
 *      Author: kottmanj
 */

#include"CCStructures.h"
#include<madness/chem/CCPotentials.h>

namespace madness {

void
CCMessenger::output(const std::string& msg) const {
    if (scientific) std::cout << std::scientific;
    else std::cout << std::fixed;

    std::cout << std::setprecision(output_prec);
    if (world.rank() == 0) std::cout << msg << std::endl;
}

void
CCMessenger::section(const std::string& msg) const {
    if (world.rank() == 0) {
        std::cout << "\n" << std::setw(msg.size() + 10) << std::setfill('*') << "\n";
        std::cout << std::setfill(' ');
        output(msg);
        std::cout << std::setw(msg.size() + 10) << std::setfill('*') << "\n\n";
        std::cout << std::setfill(' ');
    }
}

void
CCMessenger::subsection(const std::string& msg) const {
    if (world.rank() == 0) {
        std::cout << "\n" << std::setw(msg.size() + 5) << std::setfill('-') << "\n";
        std::cout << std::setfill(' ');
        output(msg);
        std::cout << std::setw(msg.size() + 5) << std::setfill('-') << "\n";
        std::cout << std::setfill(' ');
    }
}

void
CCMessenger::warning(const std::string& msg) const {
    std::string tmp = "!!!!!WARNING:" + msg + "!!!!!!";
    output(tmp);
    warnings.push_back(msg);
}

void
CCTimer::info(const bool debug, const double norm) {
    if (debug == true) {
        update_time();
        std::string s_norm = "";
        if (norm != 12345.6789) s_norm = ", ||result||=" + std::to_string(norm);

        if (world.rank() == 0) {
            std::cout << std::setfill(' ') << std::scientific << std::setprecision(2)
                      << "Timer: " << time_wall << " (Wall), " << time_cpu << " (CPU)" << s_norm
                      << ", (" + operation + ")" << "\n";
        }
    }
}

void
CCFunction::info(World& world, const std::string& msg) const {
    if (world.rank() == 0) {
        std::cout << "Information about 3D function: " << name() << " " << msg << std::endl;
        std::cout << std::setw(10) << std::setfill(' ') << std::setw(50) << " |f|    : " << function.norm2()
                  << std::endl;
        std::cout << std::setw(10) << std::setfill(' ') << std::setw(50) << " |error|: " << current_error << std::endl;
    }
}

std::string
CCFunction::name() const {
    if (type == HOLE) {
        return "phi" + stringify(i);
    } else if (type == PARTICLE) {
        return "tau" + stringify(i);
    } else if (type == MIXED) {
        return "t" + stringify(i);
    } else if (type == RESPONSE) {
        return "x" + stringify(i);
    } else {
        return "function" + stringify(i);
    }
}

madness::CC_vecfunction
CC_vecfunction::copy() const {
    std::vector<CCFunction> vn;
    for (auto x : functions) {
        const CCFunction fn(madness::copy(x.second.function), x.second.i, x.second.type);
        vn.push_back(fn);
    }
    CC_vecfunction result(vn, type);
    result.irrep = irrep;
    return result;
}

std::string
CC_vecfunction::name() const {
    if (type == PARTICLE) return "tau";
    else if (type == HOLE) return "phi";
    else if (type == MIXED) return "t";
    else if (type == RESPONSE) {
        if (excitation < 0) MADNESS_EXCEPTION("EXCITATION VECTOR HAS NO NUMBER ASSIGNED!", 1);
        return std::to_string(excitation) + "_" + "x";
    } else return "UNKNOWN";
}

void
CC_vecfunction::print_size(const std::string& msg) const {
    if (functions.size() == 0) {
        std::cout << "CC_vecfunction " << msg << " is empty\n";
    } else {
        std::string msg2;
        if (msg == "!?not assigned!?") msg2 = "";
        else msg2 = "_(" + msg + ")";

        for (auto x : functions) {
            x.second.function.print_size(x.second.name() + msg2);
        }
    }
}

void
CCPair::info() const {
    if (constant_part.world().rank() == 0) {
        std::cout << "\nInformation about electron pair: " << name() << "\n";
    }
    constant_part.print_size("ConstantPart");
    for (size_t k = 0; k < functions.size(); k++)
        functions[k].print_size();
    if (constant_part.world().rank() == 0) {
        std::cout << "\n";
    }
}

madness::vector_real_function_3d
CCIntermediatePotentials::operator()(const CC_vecfunction& f, const PotentialType& type) const {
    output("Getting " + assign_name(type) + " for " + f.name());
    vector_real_function_3d result;
    if (type == POT_singles_ and (f.type == PARTICLE or f.type == MIXED)) return current_singles_potential_gs_;
    else if (type == POT_singles_ and f.type == RESPONSE) return current_singles_potential_ex_;
    else if (type == POT_s2b_ and f.type == PARTICLE) return current_s2b_potential_gs_;
    else if (type == POT_s2b_ and f.type == RESPONSE) return current_s2b_potential_ex_;
    else if (type == POT_s2c_ and f.type == PARTICLE) return current_s2c_potential_gs_;
    else if (type == POT_s2c_ and f.type == RESPONSE) return current_s2c_potential_ex_;
    else if (f.type == HOLE) {
        output(assign_name(type) + " is zero for HOLE states");
        result = zero_functions<double, 3>(world, f.size());
    } else {
        output("ERROR: Potential was not supposed to be stored");
        MADNESS_EXCEPTION("Potential was not supposed to be stored", 1);
    }

    if (result.empty()) output("!!!WARNING: Potential is empty!!!");

    return result;
}

madness::real_function_3d
CCIntermediatePotentials::operator()(const CCFunction& f, const PotentialType& type) const {
    output("Getting " + assign_name(type) + " for " + f.name());
    real_function_3d result = real_factory_3d(world);
    if (type == POT_singles_ and (f.type == PARTICLE or f.type == MIXED))
        return current_singles_potential_gs_[f.i - parameters.freeze()];
    else if (type == POT_singles_ and f.type == RESPONSE) return current_singles_potential_ex_[f.i - parameters.freeze()];
    else if (type == POT_s2b_ and f.type == PARTICLE) return current_s2b_potential_gs_[f.i - parameters.freeze()];
    else if (type == POT_s2b_ and f.type == RESPONSE) return current_s2b_potential_ex_[f.i - parameters.freeze()];
    else if (type == POT_s2c_ and f.type == PARTICLE) return current_s2c_potential_gs_[f.i - parameters.freeze()];
    else if (type == POT_s2c_ and f.type == RESPONSE) return current_s2c_potential_ex_[f.i - parameters.freeze()];
    else if (f.type == HOLE) output(assign_name(type) + " is zero for HOLE states");
    else MADNESS_EXCEPTION("Potential was not supposed to be stored", 1)

    ;
    if (result.norm2() < FunctionDefaults<3>::get_thresh())
        output("WARNING: Potential seems to be zero ||V||=" + std::to_string(double(result.norm2())));

    return result;
}

void
CCIntermediatePotentials::insert(const vector_real_function_3d& potential, const CC_vecfunction& f,
                                 const PotentialType& type) {
    output("Storing potential: " + assign_name(type) + " for " + f.name());
    MADNESS_ASSERT(!potential.empty());
    if (type == POT_singles_ && (f.type == PARTICLE || f.type == MIXED)) current_singles_potential_gs_ = potential;
    else if (type == POT_singles_ && f.type == RESPONSE) current_singles_potential_ex_ = potential;
    else if (type == POT_s2b_ && f.type == PARTICLE) current_s2b_potential_gs_ = potential;
    else if (type == POT_s2b_ && f.type == RESPONSE) current_s2b_potential_ex_ = potential;
    else if (type == POT_s2c_ && f.type == PARTICLE) {
        current_s2c_potential_gs_ = potential;
    } else if (type == POT_s2c_ && f.type == RESPONSE) {
        current_s2c_potential_ex_ = potential;
    }
}

void CCParameters::set_derived_values() {
    if (not kain()) set_derived_value("kain_subspace",std::size_t(0));

    // set all parameters that were not explicitly given
    set_derived_value("tight_thresh_6d",thresh_6D()*0.1);
    set_derived_value("thresh_3d",thresh_6D()*0.01);
    set_derived_value("tight_thresh_3d",thresh_3D()*0.1);
//    if (thresh_operators == uninitialized) thresh_operators = 1.e-6;
//    if (thresh_operators_3D == uninitialized) thresh_operators_3D = thresh_operators;
//    if (thresh_operators_6D == uninitialized) thresh_operators_6D = thresh_operators;
//    if (thresh_bsh_3D == uninitialized) thresh_bsh_3D = thresh_operators_3D;
//    if (thresh_bsh_6D == uninitialized) thresh_bsh_6D = thresh_operators_6D;
//    if (thresh_poisson == uninitialized) thresh_poisson = thresh_operators_3D;
//    if (thresh_f12 == uninitialized) thresh_f12 = thresh_operators_3D;
    set_derived_value("thresh_ue",tight_thresh_6D());
    set_derived_value("dconv_6d",thresh_6D());
    set_derived_value("dconv_3d",thresh_6D());
    set_derived_value("econv",0.1*dconv_6D());
    set_derived_value("econv_pairs",econv());

    set_derived_value("no_compute_gs",no_compute());
    set_derived_value("no_compute_mp2",no_compute() and no_compute_gs());
    set_derived_value("no_compute_cc2",no_compute() and no_compute_gs());
    set_derived_value("no_compute_cispd",no_compute() and no_compute_response());
    set_derived_value("no_compute_response",no_compute());
    set_derived_value("restart",no_compute() == true and restart() == false);

    if (thresh_3D() < 1.1e-1) set_derived_value("output_prec",std::size_t(3));
    if (thresh_3D() < 1.1e-2) set_derived_value("output_prec",std::size_t(4));
    if (thresh_3D() < 1.1e-3) set_derived_value("output_prec",std::size_t(5));
    if (thresh_3D() < 1.1e-4) set_derived_value("output_prec",std::size_t(6));
    if (thresh_3D() < 1.1e-5) set_derived_value("output_prec",std::size_t(7));
    if (thresh_3D() < 1.1e-6) set_derived_value("output_prec",std::size_t(8));
    std::cout.precision(output_prec());
}

void CCParameters::information(World& world) const {
    if (world.rank()==0) {
//        print("cc2","end");
        if (calc_type() != CT_LRCCS and calc_type() != CT_TDHF) {
            std::cout << "The Ansatz for the Pair functions |tau_ij> is: ";
            if (QtAnsatz()) std::cout << "(Qt)f12|titj> and response: (Qt)f12(|tixj> + |xitj>) - (OxQt + QtOx)f12|titj>";
            else std::cout << "Qf12|titj> and response: Qf12(|xitj> + |tixj>)" << std::endl;
        }
    }
}

void CCParameters::sanity_check(World& world) const {
    size_t warnings = 0;
    if (FunctionDefaults<3>::get_thresh() > 0.01 * FunctionDefaults<6>::get_thresh())
        warnings += warning(world, "3D Thresh is too low, should be 0.01*6D_thresh");
    if (FunctionDefaults<3>::get_thresh() > 0.1 * FunctionDefaults<6>::get_thresh())
        warnings += warning(world, "3D Thresh is way too low, should be 0.01*6D_thresh");
    if (FunctionDefaults<3>::get_cell_min_width() != FunctionDefaults<6>::get_cell_min_width())
        warnings += warning(world, "3D and 6D Cell sizes differ");
    if (FunctionDefaults<3>::get_k() != FunctionDefaults<6>::get_k())
        warnings += warning(world, "k-values of 3D and 6D differ ");
    if (FunctionDefaults<3>::get_truncate_mode() != 3) warnings += warning(world, "3D Truncate mode is not 3");
    if (FunctionDefaults<6>::get_truncate_mode() != 3) warnings += warning(world, "6D Truncate mode is not 3");
    if (dconv_3D() < FunctionDefaults<3>::get_thresh())
        warnings += warning(world, "Demanded higher convergence than threshold for 3D");
    if (dconv_6D() < FunctionDefaults<6>::get_thresh())
        warnings += warning(world, "Demanded higher convergence than threshold for 6D");
    if (thresh_3D() != FunctionDefaults<3>::get_thresh())
        warnings += warning(world, "3D thresh set unequal 3D thresh demanded");
    if (thresh_6D() != FunctionDefaults<6>::get_thresh())
        warnings += warning(world, "6D thresh set unequal 6D thresh demanded");
    if (econv() < FunctionDefaults<3>::get_thresh())
        warnings += warning(world, "Demanded higher energy convergence than threshold for 3D");
    if (econv() < FunctionDefaults<6>::get_thresh())
        warnings += warning(world, "Demanded higher energy convergence than threshold for 6D");
    if (econv() < 0.1 * FunctionDefaults<3>::get_thresh())
        warnings += warning(world,
                            "Demanded higher energy convergence than threshold for 3D (more than factor 10 difference)");
    if (econv() < 0.1 * FunctionDefaults<6>::get_thresh())
        warnings += warning(world,
                            "Demanded higher energy convergence than threshold for 6D (more than factor 10 difference)");
    // Check if the 6D thresholds are not too high
    if (thresh_6D() < 1.e-3) warnings += warning(world, "thresh_6D is smaller than 1.e-3");
    if (thresh_6D() < tight_thresh_6D()) warnings += warning(world, "tight_thresh_6D is larger than thresh_6D");
    if (thresh_6D() < tight_thresh_3D()) warnings += warning(world, "tight_thresh_3D is larger than thresh_3D");
    if (thresh_6D() < 1.e-3) warnings += warning(world, "thresh_6D is smaller than 1.e-3");
    if (thresh_Ue() < 1.e-4) warnings += warning(world, "thresh_Ue is smaller than 1.e-4");
    if (thresh_Ue() > 1.e-4) warnings += warning(world, "thresh_Ue is larger than 1.e-4");
    if (thresh_3D() > 0.01 * thresh_6D())
        warnings += warning(world, "Demanded 6D thresh is to precise compared with the 3D thresh");
    if (thresh_3D() > 0.1 * thresh_6D())
        warnings += warning(world, "Demanded 6D thresh is to precise compared with the 3D thresh");
    if (kain() and kain_subspace() == 0)
        warnings += warning(world, "Demanded Kain solver but the size of the iterative subspace is set to zero");
    if (warnings > 0) {
        if (world.rank() == 0) std::cout << warnings << "Warnings in parameters sanity check!\n\n";
    } else {
        if (world.rank() == 0) std::cout << "Sanity check for parameters passed\n\n" << std::endl;
    }
    if (restart() == false and no_compute() == true) {
        warnings += warning(world, "no_compute flag detected but no restart flag");
    }
}

real_function_3d
CCConvolutionOperator::operator()(const CCFunction& bra, const CCFunction& ket, const bool use_im) const {
    real_function_3d result;
    if (not use_im) {
        if (world.rank() == 0)
            std::cout << "Recalculating <" << bra.name() << "|" << assign_name(operator_type) << "|" << ket.name()
                      << ">\n";
        result = ((*op)(bra.function * ket.function)).truncate();
    } else if (bra.type == HOLE and ket.type == HOLE and not imH.allpairs.empty()) result = imH(bra.i, ket.i);
    else if (bra.type == HOLE and ket.type == RESPONSE and not imR.allpairs.empty()) result = imR(bra.i, ket.i);
    else if (bra.type == HOLE and ket.type == PARTICLE and not imP.allpairs.empty()) result = imP(bra.i, ket.i);
    else if (bra.type == HOLE and ket.type == MIXED and (not imP.allpairs.empty() and not imH.allpairs.empty()))
        result = (imH(bra.i, ket.i) + imP(bra.i, ket.i));
    else {
        //if(world.rank()==0) std::cout <<"No Intermediate found for <" << bra.name()<<"|"<<assign_name(operator_type) <<"|"<<ket.name() <<"> ... recalculate \n";
        MADNESS_ASSERT(op);
        result = ((*op)(bra.function * ket.function)).truncate();
    }
    return result;
}

real_function_6d CCConvolutionOperator::operator()(const real_function_6d& u, const size_t particle) const {
    MADNESS_ASSERT(particle == 1 or particle == 2);
    MADNESS_ASSERT(operator_type == OT_G12);
    MADNESS_ASSERT(op);
    op->particle() = particle;
    return (*op)(u);
}

real_function_3d
CCConvolutionOperator::operator()(const CCFunction& bra, const real_function_6d& u, const size_t particle) const {
    MADNESS_ASSERT(particle == 1 or particle == 2);
    MADNESS_ASSERT(operator_type == OT_G12);
    MADNESS_ASSERT(op);
    const real_function_6d tmp = multiply(copy(u), copy(bra.function), particle);
    op->particle() = particle;
    const real_function_6d g_tmp = (*op)(tmp);
    const real_function_3d result = g_tmp.dirac_convolution<3>();
    return result;
}


void CCConvolutionOperator::update_elements(const CC_vecfunction& bra, const CC_vecfunction& ket) {
    const std::string operation_name = "<" + assign_name(bra.type) + "|" + name() + "|" + assign_name(ket.type) + ">";
    if (world.rank() == 0)
        std::cout << "updating operator elements: " << operation_name << " (" << bra.size() << "x" << ket.size() << ")"
                  << std::endl;
    if (bra.type != HOLE)
        error("Can not create intermediate of type " + operation_name + " , bra-element has to be of type HOLE");
    op.reset(init_op(operator_type, parameters));
    intermediateT xim;
    for (auto tmpk : bra.functions) {
        const CCFunction& k = tmpk.second;
        for (auto tmpl : ket.functions) {
            const CCFunction& l = tmpl.second;
            real_function_3d kl = (bra(k).function * l.function);
            real_function_3d result = ((*op)(kl)).truncate();
            result.reconstruct(); // for sparse multiplication
            xim.insert(k.i, l.i, result);
        }
    }
    if (ket.type == HOLE) imH = xim;
    else if (ket.type == PARTICLE) imP = xim;
    else if (ket.type == RESPONSE) imR = xim;
    else error("Can not create intermediate of type <" + assign_name(bra.type) + "|op|" + assign_name(ket.type) + ">");
}


void CCConvolutionOperator::clear_intermediates(const FuncType& type) {
    if (world.rank() == 0)
        std::cout << "Deleting all <HOLE|" << name() << "|" << assign_name(type) << "> intermediates \n";
    switch (type) {
        case HOLE : {
            imH.allpairs.clear();
            break;
        }
        case PARTICLE: {
            imP.allpairs.clear();
            break;
        }
        case RESPONSE: {
            imR.allpairs.clear();
            break;
        }
        default:
            error("intermediates for " + assign_name(type) + " are not defined");
    }
}

size_t CCConvolutionOperator::info() const {
    const size_t size_imH = size_of(imH);
    const size_t size_imP = size_of(imP);
    const size_t size_imR = size_of(imR);
    if (world.rank() == 0) {
        std::cout << "Size of " << name() << " intermediates:\n";
        std::cout << std::setw(5) << "(" << imH.allpairs.size() << ") x <H|" + name() + "H>=" << std::scientific
                  << std::setprecision(1) << size_imH << " (Gbyte)\n";
        std::cout << std::setw(5) << "(" << imP.allpairs.size() << ") x <H|" + name() + "P>=" << std::scientific
                  << std::setprecision(1) << size_imH << " (Gbyte)\n";
        std::cout << std::setw(5) << "(" << imR.allpairs.size() << ") x <H|" + name() + "R>=" << std::scientific
                  << std::setprecision(1) << size_imH << " (Gbyte)\n";
    }
    return size_imH + size_imP + size_imR;
}

SeparatedConvolution<double, 3> *
CCConvolutionOperator::init_op(const OpType& type, const Parameters& parameters) const {
    switch (type) {
        case OT_G12 : {
            if (world.rank() == 0)
                std::cout << "Creating " << assign_name(type) << " Operator with thresh=" << parameters.thresh_op
                          << " and lo=" << parameters.lo << std::endl;
            return CoulombOperatorPtr(world, parameters.lo, parameters.thresh_op);
        }
        case OT_F12 : {
            if (world.rank() == 0)
                std::cout << "Creating " << assign_name(type) << " Operator with thresh=" << parameters.thresh_op
                          << " and lo=" << parameters.lo << " and Gamma=" << parameters.gamma << std::endl;
            return SlaterF12OperatorPtr(world, parameters.gamma, parameters.lo, parameters.thresh_op);
        }
        case OT_SLATER : {
            if (world.rank() == 0)
                std::cout << "Creating " << assign_name(type) << " Operator with thresh=" << parameters.thresh_op
                          << " and lo=" << parameters.lo << " and Gamma=" << parameters.gamma << std::endl;
            return SlaterOperatorPtr(world, parameters.gamma, parameters.lo, parameters.thresh_op);
        }
        case OT_BSH : {
            if (world.rank() == 0)
                std::cout << "Creating " << assign_name(type) << " Operator with thresh=" << parameters.thresh_op
                          << " and lo=" << parameters.lo << " and Gamma=" << parameters.gamma << std::endl;
            return BSHOperatorPtr3D(world, parameters.gamma, parameters.lo, parameters.thresh_op);
        }
        case OT_ONE : {
            if (world.rank() == 0)
                std::cout << "Creating " << assign_name(type) << " Operator " << std::endl;
            return nullptr;
        }
        default : {
            error("Unknown operatorype " + assign_name(type));
            MADNESS_EXCEPTION("error", 1);
        }
    }

}

/// Assigns strings to enums for formated output
std::string
assign_name(const PairFormat& input) {
    switch (input) {
        case PT_FULL:
            return "full";
        case PT_DECOMPOSED:
            return "decomposed";
        case PT_OP_DECOMPOSED:
            return "operator-decomposed";
        default: {
            MADNESS_EXCEPTION("Unvalid enum assignement!", 1);
            return "undefined";
        }
    }
    MADNESS_EXCEPTION("assign_name:pairtype, should not end up here", 1);
    return "unknown pairtype";
}

/// Assigns strings to enums for formated output
std::string
assign_name(const CCState& input) {
    switch (input) {
        case GROUND_STATE:
            return "Ground State";
        case EXCITED_STATE:
            return "Excited State";
        default: {
            MADNESS_EXCEPTION("Unvalid enum assignement!", 1);
            return "undefined";
        }
    }
    MADNESS_EXCEPTION("assign_name:pairtype, should not end up here", 1);
    return "unknown pairtype";
}

/// Assigns strings to enums for formated output
std::string
assign_name(const OpType& input) {
    switch (input) {
        case OT_G12:
            return "g12";
        case OT_F12:
            return "f12";
        case OT_SLATER:
            return "slater";
        case OT_FG12:
            return "fg12";
        case OT_BSH:
            return "bsh";
        case OT_ONE:
            return "identity";
        default: {
            MADNESS_EXCEPTION("Unvalid enum assignement!", 1);
            return "undefined";
        }
    }
    MADNESS_EXCEPTION("assign_name:optype, should not end up here", 1);
    return "unknown operatortype";
}

/// Assigns enum to string
CalcType
assign_calctype(const std::string name) {
    if (name == "mp2") return CT_MP2;
    else if (name == "cc2") return CT_CC2;
    else if (name == "lrcc2" or name == "cc2_response") return CT_LRCC2;
    else if (name == "cispd") return CT_CISPD;
    else if (name == "cis" or name == "ccs" or name == "ccs_response" or name == "lrccs") return CT_LRCCS;
    else if (name == "experimental") return CT_TEST;
    else if (name == "adc2" or name == "adc(2)") return CT_ADC2;
    else if (name == "tdhf") return CT_TDHF;
    else {
        std::string msg = "CALCULATION OF TYPE: " + name + " IS NOT KNOWN!!!!";
        MADNESS_EXCEPTION(msg.c_str(), 1);
    }
}

/// Assigns strings to enums for formated output
std::string
assign_name(const CalcType& inp) {
    switch (inp) {
        case CT_CC2:
            return "CC2";
        case CT_MP2:
            return "MP2";
        case CT_LRCC2:
            return "LRCC2";
        case CT_CISPD:
            return "CISpD";
        case CT_LRCCS:
            return "LRCCS";
        case CT_ADC2:
            return "ADC2";
        case CT_TDHF:
            return "TDHF";
        case CT_TEST:
            return "experimental";
        default: {
            MADNESS_EXCEPTION("Unvalid enum assignement!", 1);
            return "undefined";
        }
    }
    return "unknown";
}

/// Assigns strings to enums for formated output
std::string
assign_name(const PotentialType& inp) {
    switch (inp) {
        case POT_F3D_:
            return "F3D";
        case POT_s3a_:
            return "s3a";
        case POT_s3b_:
            return "s3b";
        case POT_s3c_:
            return "s3c";
        case POT_s5a_:
            return "s5a";
        case POT_s5b_:
            return "s5b";
        case POT_s5c_:
            return "s5c";
        case POT_s6_:
            return "s6";
        case POT_s2b_:
            return "s2b";
        case POT_s2c_:
            return "s2c";
        case POT_s4a_:
            return "s4a";
        case POT_s4b_:
            return "s4b";
        case POT_s4c_:
            return "s4c";
        case POT_ccs_:
            return "ccs";
        case POT_cis_:
            return "cis-potential";
        case POT_singles_:
            return "singles potential";
        default: {
            MADNESS_EXCEPTION("Unvalid enum assignement!", 1);
            return "undefined";
        }
    }
    return "undefined";
}

/// Assigns strings to enums for formated output
std::string
assign_name(const FuncType& inp) {
    switch (inp) {
        case HOLE:
            return "Hole";
        case PARTICLE:
            return "Particle";
        case MIXED:
            return "Mixed";
        case RESPONSE:
            return "Response";
        case UNDEFINED:
            return "Undefined";
        default: {
            MADNESS_EXCEPTION("Unvalid enum assignement!", 1);
            return "undefined";
        }
    }
    return "???";
}

/// Returns the size of an intermediate
double
size_of(const intermediateT& im) {
    double size = 0.0;
    for (const auto& tmp : im.allpairs) {
        size += get_size<double, 3>(tmp.second);
    }
    return size;
}

std::vector<real_function_6d>
MacroTaskMp2ConstantPart::operator() (const std::vector<CCPair>& pair, const std::vector<real_function_3d>& mo_ket,
                                      const std::vector<real_function_3d>& mo_bra, const CCParameters& parameters,
                                      const real_function_3d& Rsquare, const std::vector<real_function_3d>& U1,
                                      const std::vector<std::string>& argument) const {
    World& world = mo_ket[0].world();
    resultT result = zero_functions_compressed<double, 6>(world, pair.size());
    for (int i = 0; i < pair.size(); i++) {
        result[i] = CCPotentials::make_constant_part_mp2_macrotask(world, pair[i], mo_ket, mo_bra, parameters,
                                                                   Rsquare, U1, argument);
    }
    return result;
}

std::vector<real_function_6d>
MacroTaskMp2UpdatePair::operator() (const std::vector<CCPair> &pair,
                                    const std::vector<real_function_6d> &mp2_coupling,
                                    const CCParameters &parameters,
                                    const std::vector<madness::Vector<double, 3>> &all_coords_vec,
                                    const std::vector<real_function_3d> &mo_ket,
                                    const std::vector<real_function_3d> &mo_bra,
                                    const std::vector<real_function_3d> &U1, const real_function_3d &U2) const {
    World& world = mo_ket[0].world();
    resultT result = zero_functions_compressed<double, 6>(world, pair.size());

    for (int i = 0; i < pair.size(); i++) {
        //(i, j) -> j*(j+1) + i
        result[i] = CCPotentials::update_pair_mp2_macrotask(world, pair[i], parameters, all_coords_vec, mo_ket,
                                                            mo_bra, U1, U2, mp2_coupling[i]);
    }
    return result;
}

}// end namespace madness


