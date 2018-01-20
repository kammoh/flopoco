#include "Primitive.hpp"
// general c++ library for manipulating streams
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "gmp.h"
#include "mpfr.h"
#include "Target.hpp"

using namespace std;
namespace flopoco {
    Primitive::Primitive( Target *target ) : Operator( target ){
        setCopyrightString( UniKs::getAuthorsString( UniKs::AUTHOR_MKLEINLEIN ) );
        setCombinatorial();
    }

    Primitive::~Primitive() {}

    void Primitive::addPrimitiveLibrary(OperatorPtr op, Target *target ){
        const std::string copyr = op->getCopyrightString();
        // Does bad things to copyright string...
        std::stringstream o;
        o << std::endl << "--------------------------------------------------------------------------------" << std::endl;
        if( target->getVendor() == "Xilinx" ){
            if( target->getID() == "Virtex6" ){
                o << "library UNISIM;" << std::endl;
                o << "use UNISIM.Vcomponents.all;";
            }else {
                throw std::runtime_error("Target not supported for primitives");
            }
        }else if(target->getVendor() == "Altera"){
            o << "library wysiwyg;" << std::endl;
            o << "use wysiwyg.";
            if( target->getID() == "CycloneII" ){
                o << "cycloneii";
            }else if( target->getID() == "CycloneIII" ){
                o << "cycloneiii";
            }else if( target->getID() == "CycloneIV" ){
                o << "cycloneiv";
            }else if( target->getID() == "CycloneV" ){
                o << "cyclonev";
            }else if( target->getID() == "StratixII" ){
                o << "stratixii";
            }else if( target->getID() == "StratixIII" ){
                o << "stratixiii";
            }else if( target->getID() == "StratixIV" ){
                o << "stratixiv";
            }else if( target->getID() == "StratixV" ){
                o << "stratixv";
            }else{
                throw std::runtime_error("Target not supported for primitives");
            }
            o << "_components.all;" << std::endl;
        }else{
            throw std::runtime_error("Target not supported for primitives");
        }
        if( copyr.find( o.str() )==std::string::npos  ){
            op->setCopyrightString( copyr + o.str() );
        }
    }

    void Primitive::checkTargetCompatibility( Target *target ) {
        if( target->getVendor() != "Xilinx" || target->getID() != "Virtex6" ) {
            throw std::runtime_error( "This component is only suitable for target Xilinx Virtex6 and above." );
        }
    }

    void Primitive::setGeneric( string name, string value ) {
        generics_.insert( std::make_pair( name, value ) );
    }

    void Primitive::setGeneric( string name, const long value ) {
        setGeneric( name, std::to_string( value ) );
    }

    std::map<string, string> &Primitive::getGenerics() {
        return generics_;
    }

    void Primitive::outputVHDL( ostream &o, string name ) {
    }

    void Primitive::outputVHDLComponent( ostream &o, string name ) {
    }

    string Primitive::primitiveInstance( string instanceName , OperatorPtr parent) {
        Primitive *op = this;

        // INSERTED PART FOR PRIMITIVES
        if( parent != nullptr )
            addPrimitiveLibrary(parent,parent->getTarget());
        // INSERTED PART END

        ostringstream o;
#if 0 //!!
        o << tab << instanceName << ": " << op->getName();

        if ( op->isSequential() ) {
            o << "  -- pipelineDepth=" << op->getPipelineDepth(); // << " maxInDelay=" << getMaxInputDelays( op->getInputDelayMap() );
        }

        o << endl;

        // INSERTED PART FOR PRIMITIVES
        if( !getGenerics().empty() ) {
            o << tab << tab << "generic map ( ";
            std::map<string, string>::iterator it = getGenerics().begin();
            o << it->first << " => " << it->second;

            for( ++it; it != getGenerics().end(); ++it  ) {
                o << "," << endl << tab << tab << it->first << " => " << it->second;
            }

            o << ")" << endl;
        }
        // INSERTED PART END

        o << tab << tab << "port map ( ";
        // build vhdl and erase portMap_
        map<string, string>::iterator it;

        if( op->isSequential() ) {
            o << "clk  => clk";
            o << "," << endl << tab << tab << "           rst  => rst";

            if ( op->isRecirculatory() ) {
                o << "," << endl << tab << tab << "           stall_s => stall_s";
            };

            if ( op->hasClockEnable() ) {
                o << "," << endl << tab << tab << "           ce => ce";
            };
        }

        map<string, string> portmap = op->getPortMap();
        for ( it = portmap.begin()  ; it != portmap.end(); ++it ) {
            bool outputSignal = false;
            const string port_name = ( *it ).first;
            for ( int k = 0; k < int( op->getIOList()->size() ); k++ ) {
                const string io_name = op->getIOList()->at( k )->getName();
                if ( ( op->getIOList()->at( k )->type() == Signal::out ) && (io_name  == port_name ) ) {
                    outputSignal = true;
                }
            }

            bool parsing = vhdl.isParsing();

            if ( outputSignal && parsing ) {
                vhdl.flush( getCurrentCycle() );
                vhdl.disableParsing( true );
            }

            if ( it != portmap.begin() || op->isSequential() ) {
                o << "," << endl <<  tab << tab << "           ";
            }

            // The following code assumes that the IO is declared as standard_logic_vector
            // If the actual parameter is a signed or unsigned, we want to automatically convert it
            Signal *rhs;
            string rhsString;

            // The following try was intended to distinguish between variable and constant
            // but getSignalByName doesn't catch delayed variables
            try {
                //cout << "its = " << (*it).second << "  " << endl;
                rhs = getDelayedSignalByName( ( *it ).second );

                if ( rhs->isFix() && !outputSignal ) {
                    rhsString = std_logic_vector( ( *it ).second );
                } else {
                    rhsString = ( *it ).second;
                }
            } catch( string e ) {
                //constant here
                rhsString = ( *it ).second;
            }

            o << ( *it ).first << " => " << rhsString;

            if ( outputSignal && parsing ) {
                vhdl << o.str();
                vhdl.flush( getCurrentCycle() );
                o.str( "" );
                vhdl.disableParsing( !parsing );
            }

            //op->portMap_.erase(it);
        }

        o << ");" << endl;
        /*
                //Floorplanning related-----------------------------------------
                floorplan << manageFloorplan();
                flpHelper->addToFlpComponentList(op->getName());
                flpHelper->addToInstanceNames(op->getName(), instanceName);
                //--------------------------------------------------------------

        */
        // add the operator to the subcomponent list: still explicit in origin/master
        //
        // subComponents_.push_back(op);

#endif //!!
        return o.str();
    }

    string UniKs::getAuthorsString( const int &authors ) {
        std::stringstream out;
        out << std::endl;
        out << "-- >> University of Kassel, Germany" << std::endl;
        out << "-- >> Digital Technology Group" << std::endl;
        out << "-- >> Author(s):" << endl;
        bool needs_newline = false;
        if( authors & AUTHOR_MKUMM ) {
            out << "-- >> Martin Kumm <kumm@uni-kassel.de>";
            needs_newline = true;
        }

        if( authors & AUTHOR_KMOELLER ) {
            if( needs_newline ) out << std::endl;
            out << "-- >> Konrad Möller <konrad.moeller@uni-kassel.de>";
            needs_newline = true;
        }

        if( authors & AUTHOR_JKAPPAUF ) {
            if( needs_newline ) out << std::endl;
            out << "-- >> Johannes Kappauf <uk009669@student.uni-kassel.de>";
            needs_newline = true;
        }

        if( authors & AUTHOR_MKLEINLEIN ) {
            if( needs_newline ) out << std::endl;
            out << "-- >> Marco Kleinlein <kleinlein@uni-kassel.de>";
            needs_newline = true;
        }

        return out.str();
    }

}//namespace
