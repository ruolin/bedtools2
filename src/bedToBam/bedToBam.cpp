/*****************************************************************************
  bedToBam.cpp

  (c) 2009 - Aaron Quinlan
  Hall Laboratory
  Department of Biochemistry and Molecular Genetics
  University of Virginia
  aaronquinlan@gmail.com

  Licenced under the GNU General Public License 2.0 license.
******************************************************************************/
#include "lineFileUtilities.h"
#include "bedFile.h"
#include "GenomeFile.h"
#include "version.h"
/*RL: add header*/
#include "Fasta.h"
#include "BlockedIntervals.h"

#include "api/BamReader.h"
#include "api/BamAux.h"
#include "api/BamWriter.h"
using namespace BamTools;

#include <vector>
#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace std;


/*RL define enum*/
enum Orient{
    S1 = 0,
    S2,
    A1,
    A2
};

Orient hashit (std::string const& inString){
    if(inString == "S/1") return S1;
    if(inString == "S/2") return S2;
    if(inString == "A/1") return A1;
    if(inString == "A/2") return A2;
}

// define our program name
#define PROGRAM_NAME "bedtools bedtobam"

// define our parameter checking macro
#define PARAMETER_CHECK(param, paramLen, actualLen) (strncmp(argv[i], param, min(actualLen, paramLen))== 0) && (actualLen == paramLen)


// function declarations
void bedtobam_help(void);

/*RL: Change of function definition*/
void ProcessBed(BedFile *bed, GenomeFile *genome, const string &dbfile, bool isBED12, int mapQual, bool uncompressedBam);
void ConvertBedToBam(const BED &bed, const string &seq, const uint32_t &flag, const int32_t &matePos, BamAlignment &bam, map<string, int> &chromToId, bool isBED12, int mapQual, int lineNum);

void MakeBamHeader(const string &genomeFile, RefVector &refs, string &header, map<string, int> &chromToInt);
int  bedtobam_reg2bin(int beg, int end);



int bedtobam_main(int argc, char* argv[]) {
    /*
    * This function has been modified by Ruolin Liu in order to
    * generated paried-end BAM from Flux Simulator Bed file.
    */

    // our configuration variables
    bool showHelp = false;

    // input files
    string bedFile = "stdin";
    string genomeFile;
    string fastaDbFile;

    int mapQual = 255;

    bool haveBed         = true;
    bool haveGenome      = false;
    bool haveFastaDb     = false;
    bool haveMapQual     = false;
    bool isBED12         = false;
    bool uncompressedBam = false;

    for(int i = 1; i < argc; i++) {
        int parameterLength = (int)strlen(argv[i]);

        if((PARAMETER_CHECK("-h", 2, parameterLength)) ||
        (PARAMETER_CHECK("--help", 5, parameterLength))) {
            showHelp = true;
        }
    }

    if(showHelp) bedtobam_help();

    // do some parsing (all of these parameters require 2 strings)
    for(int i = 1; i < argc; i++) {

        int parameterLength = (int)strlen(argv[i]);

        if(PARAMETER_CHECK("-i", 2, parameterLength)) {
            if ((i+1) < argc) {
                bedFile = argv[i + 1];
                i++;
            }
        }

        else if(PARAMETER_CHECK("-fi", 3, parameterLength)) {
            if ((i+1) < argc) {
                haveFastaDb = true;
                fastaDbFile = argv[i + 1];
                i++;
            }
        }

        else if(PARAMETER_CHECK("-g", 2, parameterLength)) {
            if ((i+1) < argc) {
                haveGenome = true;
                genomeFile = argv[i + 1];
                i++;
            }
        }
        else if(PARAMETER_CHECK("-mapq", 5, parameterLength)) {
            haveMapQual = true;
            if ((i+1) < argc) {
                mapQual = atoi(argv[i + 1]);
                i++;
            }
        }
        else if(PARAMETER_CHECK("-bed12", 6, parameterLength)) {
            isBED12 = true;
        }
        else if(PARAMETER_CHECK("-ubam", 5, parameterLength)) {
            uncompressedBam = true;
        }
        else {
            cerr << endl << "*****ERROR: Unrecognized parameter: " << argv[i] << " *****" << endl << endl;
            showHelp = true;
        }
    }

    // make sure we have an input files
    if (!haveBed ) {
        cerr << endl << "*****" << endl << "*****ERROR: Need -i (BED) file. " << endl << "*****" << endl;
        showHelp = true;
    }
    if (!haveGenome ) {
        cerr << endl << "*****" << endl << "*****ERROR: Need -g (genome) file. " << endl << "*****" << endl;
        showHelp = true;
    }
    if (mapQual < 0 || mapQual > 255) {
        cerr << endl << "*****" << endl << "*****ERROR: MAPQ must be in range [0,255]. " << endl << "*****" << endl;
        showHelp = true;
    }


    if (!showHelp) {
        BedFile *bed       = new BedFile(bedFile);
        GenomeFile *genome = new GenomeFile(genomeFile);

        ProcessBed(bed, genome, fastaDbFile, isBED12, mapQual, uncompressedBam);
    }
    else {
        bedtobam_help();
    }
    return 0;
}


void bedtobam_help(void) {
    
    cerr << "\nTool:    bedtools bedtobam (aka bedToBam)" << endl;
    cerr << "Version: " << VERSION << "\n";
    cerr << "Summary: Converts feature records to BAM format." << endl << endl;

    cerr << "Usage:   " << PROGRAM_NAME << " [OPTIONS] -i <bed/gff/vcf> -g <genome> -fi <fasta>" << endl << endl;

    cerr << "Options: " << endl;

    cerr << "\t-mapq\t" << "Set the mappinq quality for the BAM records." << endl;
    cerr                    << "\t\t(INT) Default: 255" << endl << endl;

    cerr << "\t-bed12\t"    << "The BED file is in BED12 format.  The BAM CIGAR" << endl;
    cerr                    << "\t\tstring will reflect BED \"blocks\"." << endl << endl;

    cerr << "\t-ubam\t"     << "Write uncompressed BAM output. Default writes compressed BAM." << endl << endl;

    cerr << "Notes: " << endl;
    cerr << "\t(1)  BED files must be at least BED4 to create BAM (needs name field)." << endl << endl;


    // end the program here
    exit(1);
}


void ProcessBed(BedFile *bed, GenomeFile *genome, const string &dbfile, bool isBED12, int mapQual, bool uncompressedBam) {
   /*
    * This function has been modified by Ruolin Liu in order to
    * generated paried-end BAM from Flux Simulator Bed file.
    */

    BamWriter *writer = new BamWriter();

    // build a BAM header from the genomeFile
    RefVector refs;
    string    bamHeader;
    map<string, int, std::less<string> > chromToId;
    MakeBamHeader(genome->getGenomeFileName(), refs, bamHeader, chromToId);
        
    // set compression mode
    BamWriter::CompressionMode compressionMode = BamWriter::Compressed;
    if ( uncompressedBam ) compressionMode = BamWriter::Uncompressed;
    writer->SetCompressionMode(compressionMode);
    // open a BAM and add the reference headers to the BAM file
    writer->Open("stdout", bamHeader, refs);

    FastaReference *fr = new FastaReference;
    bool memmap = true;
    fr->open(dbfile, memmap, false);
    //cerr<<dbfile<<endl;
    // process each BED entry and convert to BAM

    BED bedEntry1; /*RL: process pair*/
    BED bedEntry2;
    BED nullBed;

    string bam_seq1; /*RL: for BAM sequence*/
    uint32_t bam_flag1 = 0; /*RL: for BAM flag*/
    string bam_seq2;
    uint32_t bam_flag2 = 0;
    // open the BED file for reading.
    bed->Open();
    while (bed->GetNextBed(bedEntry1)) {

        bed->GetNextBed(bedEntry2);
        if (bed->_status == BED_VALID) {
            /*RL EDIT BEGIN*/
            //make sure two entry pair-up
            string entry_name1 = bedEntry1.name.substr(0, bedEntry1.name.size()-4);
            string entry_name2 = bedEntry2.name.substr(0, bedEntry2.name.size()-4);
            assert(entry_name1 == entry_name2);

            bam_seq1.clear();
            bam_seq2.clear();
            size_t seqLength = fr->sequenceLength(bedEntry1.chrom);
            if(seqLength){
                // make sure this feature will not exceed
                // the end of the chromosome and not a polyA read.
                if( (bedEntry1.start <= seqLength) && (bedEntry1.end <= seqLength)
                        && (bedEntry1.chrom != "polyA"))
                {
                    int length = bedEntry1.end - bedEntry1.start;
                    bedVector bedBlocks;
                    GetBedBlocks(bedEntry1, bedBlocks);

                    for(int i = 0; i< (int) bedBlocks.size(); ++i){
                        bam_seq1 += fr->getSubSequence(bedEntry1.chrom,
                                bedBlocks[i].start,
                                bedBlocks[i].end - bedBlocks[i].start);
                    }
                }

                if( (bedEntry2.start <= seqLength) && (bedEntry2.end <= seqLength)
                        && (bedEntry2.chrom != "polyA"))
                {
                    int length = bedEntry2.end - bedEntry2.start;
                    bedVector bedBlocks;
                    GetBedBlocks(bedEntry2, bedBlocks);
                    for(int i = 0; i< (int) bedBlocks.size(); ++i){
                        bam_seq2 += fr->getSubSequence(bedEntry2.chrom,
                                bedBlocks[i].start,
                                bedBlocks[i].end - bedBlocks[i].start);
                    }
                }

            }
            string entry_flag = bedEntry1.name.substr(bedEntry1.name.size()-3, 3);
            string entry_name = bedEntry1.name.substr(0, bedEntry1.name.size()-4);
            bedEntry1.name = entry_name;
            // entry one
            switch (hashit(entry_flag)){
            // replace bedEntry.strand with reference transcript strandness
            case S1:
                if(bedEntry1.strand == "+"){
                    bedEntry1.strand = "+";
                    bam_flag1 = 99;
                }
                else{
                    bedEntry1.strand = "-";
                    bam_flag1 = 83;
                }
                break;
            case S2:
                if(bedEntry1.strand == "+"){
                    bedEntry1.strand = "+";
                    bam_flag1= 163;
                }
                else{
                    bedEntry1.strand = "-";
                    bam_flag1 =  147;
                }
                break;
            case A1:
                if(bedEntry1.strand == "+"){
                    bedEntry1.strand = "-";
                    bam_flag1 = 99;
                }
                else{
                    bedEntry1.strand = "+";
                    bam_flag1 = 83;
                }
                break;
            case A2:
                if(bedEntry1.strand == "+"){
                    bedEntry1.strand = "-";
                    bam_flag1 = 163;
                }
                else{
                    bedEntry1.strand = "+";
                    bam_flag1 = 147;
                }
                break;
            default:
                assert(false);
                break;
            }

            //entry two
            entry_flag = bedEntry2.name.substr(bedEntry2.name.size()-3, 3);
            entry_name = bedEntry2.name.substr(0, bedEntry2.name.size()-4);
            bedEntry2.name = entry_name;
            switch (hashit(entry_flag)){
            // replace bedEntry.strand with reference transcript strandness
            case S1:
                if(bedEntry2.strand == "+"){
                    bedEntry2.strand = "+";
                    bam_flag2 = 99;
                }
                else{
                    bedEntry2.strand = "-";
                    bam_flag2 = 83;
                }
                break;
            case S2:
                if(bedEntry2.strand == "+"){
                    bedEntry2.strand = "+";
                    bam_flag2= 163;
                }
                else{
                    bedEntry2.strand = "-";
                    bam_flag2 =  147;
                }
                break;
            case A1:
                if(bedEntry2.strand == "+"){
                    bedEntry2.strand = "-";
                    bam_flag2 = 99;
                }
                else{
                    bedEntry2.strand = "+";
                    bam_flag2 = 83;
                }
                break;
            case A2:
                if(bedEntry2.strand == "+"){
                    bedEntry2.strand = "-";
                    bam_flag2 = 163;
                }
                else{
                    bedEntry2.strand = "+";
                    bam_flag2 = 147;
                }
                break;
            default:
                assert(false);
                break;
            }

            BamAlignment bamEntry1;
            BamAlignment bamEntry2;
            if (bed->bedType >= 4) {

                //differentiate polyA read v.s. non-polyA reads
                if(bam_seq1.empty() || bam_seq2.empty()){
                    if( bam_seq1.empty() && bam_seq2.empty() ){
                        continue;
                    }
                    if(bam_seq1.empty()){
                        if(bedEntry2.strand == "+")
                            ConvertBedToBam(bedEntry2, bam_seq2, 0, -1, bamEntry2, chromToId, isBED12, mapQual, bed->_lineNum);
                        else
                            ConvertBedToBam(bedEntry2, bam_seq2, 16, -1, bamEntry2, chromToId, isBED12, mapQual, bed->_lineNum);

                        writer->SaveAlignment(bamEntry2);
                    }
                    else{
                        if(bedEntry1.strand == "+")
                            ConvertBedToBam(bedEntry1, bam_seq1, 0, -1, bamEntry1, chromToId, isBED12, mapQual, bed->_lineNum);
                        else
                            ConvertBedToBam(bedEntry1, bam_seq1, 16, -1, bamEntry1, chromToId, isBED12, mapQual, bed->_lineNum);
                        writer->SaveAlignment(bamEntry1);
                    }
                }
                else{
                    ConvertBedToBam(bedEntry1, bam_seq1, bam_flag1, bedEntry2.start, bamEntry1, chromToId, isBED12, mapQual, bed->_lineNum);
                    writer->SaveAlignment(bamEntry1);
                    ConvertBedToBam(bedEntry2, bam_seq2, bam_flag2, bedEntry1.start, bamEntry2, chromToId, isBED12, mapQual, bed->_lineNum);
                    writer->SaveAlignment(bamEntry2);
                }
                bedEntry1 = nullBed;
                bedEntry2 = nullBed;
            }
            /*RL: EDIT END*/
            else {
                cerr << "Error: BED entry without name found at line: " << bed->_lineNum << ".  Exiting!" << endl;
                exit (1);
            }
        }
    }
    //close up
    bed->Close();
    writer->Close();
}


void ConvertBedToBam(const BED &bed,  const string &seq, const uint32_t &flag, const int32_t &matePos, BamAlignment &bam, map<string, int, std::less<string> > &chromToId,
                     bool isBED12, int mapQual, int lineNum) {
   /*
    * This function has been modified by Ruolin Liu in order to
    * generated paried-end BAM from Flux Simulator Bed file.
    */

    bam.Name       = bed.name;
    bam.Position   = bed.start;
    bam.Bin        = bedtobam_reg2bin(bed.start, bed.end);

    // hard-code the sequence and qualities.
    int bedLength  = bed.end - bed.start;

    // set dummy seq and qual strings.  the input is BED,
    // so the sequence is inherently the same as it's
    // reference genome.
    // Thanks to James M. Ward for pointing this out.
    //bam.QueryBases = "";
    //bam.Qualities  = "";
    bam.QueryBases = seq;
    bam.Qualities = string(seq.size(), 'I');
    // chrom and map quality
    bam.RefID      = chromToId[bed.chrom];
    bam.MapQuality = mapQual;
    // set the BAM FLAG
    bam.AlignmentFlag = flag;
    //bam.AlignmentFlag = 0;
    //if (bed.strand == "-")
        //bam.SetIsReverseStrand(true);

    //bam.MatePosition = -1;
    //bam.InsertSize   = 0;
    //bam.MateRefID    = -1;

    bam.MatePosition = matePos;
    if(matePos == -1){
        bam.MateRefID = -1;
        bam.InsertSize = 0;
    }
    else{
        bam.MateRefID = chromToId[bed.chrom];
        bam.InsertSize   = matePos - bam.Position;
    }

    bam.AddTag("NM","i", 0);
    bam.AddTag("NH","i", 1);
    if(bed.strand == "+"){
        const uint8_t pos = 43;
        bam.AddTag("XS", "A", pos);
    }
    else if(bed.strand == "-"){
        const uint8_t neg = 45;
        bam.AddTag("XS", "A", neg);
    }
    else
        assert(false);


    bam.CigarData.clear();

    if (isBED12 == false) {
        CigarOp cOp;
        cOp.Type = 'M';
        cOp.Length = bedLength;
        bam.CigarData.push_back(cOp);
    }
    // we're being told that the input is BED12.
    else{

        // does it smell like BED12?  if so, process it.
        if (bed.fields.size() == 12) {

            // extract the relevant BED fields to convert BED12 to BAM
            // namely: blockCount, blockStarts, blockEnds
            unsigned int blockCount = atoi(bed.fields[9].c_str());

            vector<int> blockSizes, blockStarts;
            Tokenize(bed.fields[10], blockSizes, ',');
            Tokenize(bed.fields[11], blockStarts, ',');

            // make sure this is a well-formed BED12 entry.
            if (blockSizes.size() != blockCount) {
                cerr << "Error: Number of BED blocks does not match blockCount at line: " << lineNum << ".  Exiting!" << endl;
                exit (1);
            }
            else {
                // does the first block start after the bed.start?
                // if so, we need to do some "splicing"
                if (blockStarts[0] > 0) {
                    CigarOp cOp;
                    cOp.Length = blockStarts[0];
                    cOp.Type = 'N';
                    bam.CigarData.push_back(cOp);
                }
                // handle the "middle" blocks
                for (unsigned int i = 0; i < blockCount - 1; ++i) {
                    CigarOp cOp;
                    cOp.Length = blockSizes[i];
                    cOp.Type = 'M';
                    bam.CigarData.push_back(cOp);

                    if (blockStarts[i+1] > (blockStarts[i] + blockSizes[i])) {
                        CigarOp cOp;
                        cOp.Length = (blockStarts[i+1] - (blockStarts[i] + blockSizes[i]));
                        cOp.Type = 'N';
                        bam.CigarData.push_back(cOp);
                    }
                }
                // handle the last block.
                CigarOp cOp;
                cOp.Length = blockSizes[blockCount - 1];
                cOp.Type = 'M';
                bam.CigarData.push_back(cOp);
            }
        }
        // it doesn't smell like BED12.  complain.
        else {
            cerr << "You've indicated that the input file is in BED12 format, yet the relevant fields cannot be found.  Exiting." << endl << endl;
            exit(1);
        }
    }
}


void MakeBamHeader(const string &genomeFile, RefVector &refs, string &header,
                   map<string, int, std::less<string> > &chromToId) {

    // make a genome map of the genome file.
    GenomeFile genome(genomeFile);

    header += "@HD\tVN:1.0\tSO:unsorted\n";
    header += "@PG\tID:BEDTools_bedToBam\tVN:V";
    header += VERSION;
    header += "\n";

    int chromId = 0;
    vector<string> chromList = genome.getChromList();
    // ARQ: 23-May-2012.  No need to sort. Allow genome file to
    // drive the order of the chromosomes in the BAM header.
    // sort(chromList.begin(), chromList.end());

    // create a BAM header (@SQ) entry for each chrom in the BEDTools genome file.
    vector<string>::const_iterator genomeItr  = chromList.begin();
    vector<string>::const_iterator genomeEnd  = chromList.end();
    for (; genomeItr != genomeEnd; ++genomeItr) {
        chromToId[*genomeItr] = chromId;
        chromId++;

        // add to the header text
        int size = genome.getChromSize(*genomeItr);
        string chromLine = "@SQ\tSN:" + *genomeItr + "\tAS:" + genomeFile + "\tLN:" + ToString(size) + "\n";
        header += chromLine;

        // create a chrom entry and add it to
        // the RefVector
        RefData chrom;
        chrom.RefName            = *genomeItr;
        chrom.RefLength          = size;
        refs.push_back(chrom);
    }
}


/* Taken directly from the SAMTools spec
calculate bin given an alignment in [beg,end) (zero-based, half-close, half-open) */
int bedtobam_reg2bin(int beg, int end) {
    --end;
    if (beg>>14 == end>>14) return ((1<<15)-1)/7 + (beg>>14);
    if (beg>>17 == end>>17) return ((1<<12)-1)/7 + (beg>>17);
    if (beg>>20 == end>>20) return ((1<<9)-1)/7  + (beg>>20);
    if (beg>>23 == end>>23) return ((1<<6)-1)/7  + (beg>>23);
    if (beg>>26 == end>>26) return ((1<<3)-1)/7  + (beg>>26);
    return 0;
}


