bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_1.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_1.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_2.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_2.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_3.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_3.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_4.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_4.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_5.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_5.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_6.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_6.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_7.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_7.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_8.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_8.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_9.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_9.bam
bedToBam -bed12 -i ~/git/ASmethodsBenchmarking/RD100/RD100.control_10.bed -g arabidopsis/TAIR10.chromInfo -fi arabidopsis/whole_genome.fa > RD100/RD100.control_10.bam
samtools sort RD100/RD100.control_1.bam RD100/RD100_1
samtools sort RD100/RD100.control_2.bam RD100/RD100_2
samtools sort RD100/RD100.control_3.bam RD100/RD100_3
samtools sort RD100/RD100.control_4.bam RD100/RD100_4
samtools sort RD100/RD100.control_5.bam RD100/RD100_5
samtools sort RD100/RD100.control_6.bam RD100/RD100_6
samtools sort RD100/RD100.control_7.bam RD100/RD100_7
samtools sort RD100/RD100.control_8.bam RD100/RD100_8
samtools sort RD100/RD100.control_9.bam RD100/RD100_9
samtools sort RD100/RD100.control_10.bam RD100/RD100_10
