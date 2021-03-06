
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: ./read context_file_name.ctx\n");
    exit(-1);
  }
  // Sanity test
  printf("%lu %lu %lu %lu %lu\n", sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long double));

  /**
    Read the header
  **/
  FILE *fp = fopen(argv[1], "r");
  int read;
  char magic_number[6];
  int version;
  int kmer_size;
  int number_of_bitfields;
  int number_of_colours;
  read = fread(magic_number,sizeof(char),6,fp);
  printf("%s\n", magic_number);

  read = fread(&version,sizeof(int),1,fp);
  read = fread(&kmer_size,sizeof(int),1,fp);
  read = fread(&number_of_bitfields,sizeof(int),1,fp);
  read = fread(&number_of_colours,sizeof(int),1,fp);
  printf("%d %d %d %d\n", version, kmer_size, number_of_bitfields, number_of_colours);

  int i, j;
  int  max_tot = 0;
  for (i=0; i<number_of_colours;i++) {
    int mean_read_len=0;
    read = fread(&mean_read_len, sizeof(int),1,fp);
    long long tot=0;
    read = fread(&tot, sizeof(long long),1,fp);
    if (tot > max_tot)
      max_tot = (int) tot;
    printf("%d %lld\n", mean_read_len, tot);
  }
  for (i=0; i<number_of_colours;i++) {
    int sample_id_lens; 
    read = fread(&sample_id_lens,sizeof(int),1,fp);
    char tmp_name[sample_id_lens + 1]; // should null this out to avoid extra stack junk being printed
    read = fread(tmp_name,sizeof(char),sample_id_lens,fp);
    printf("%d %s\n", sample_id_lens, tmp_name);
  }
  for (i=0; i<number_of_colours;i++) {
    long double seq_err;
    read = fread(&seq_err,sizeof(long double),1,fp);
  }
  for (i=0; i<number_of_colours;i++) {
    char dummy;
    int d;

    read = fread(&dummy,sizeof(char),1,fp);
    read = fread(&dummy,sizeof(char),1,fp);
    read = fread(&dummy,sizeof(char),1,fp);
    read = fread(&dummy,sizeof(char),1,fp);
    read = fread(&d,sizeof(int),1,fp);
    read = fread(&d,sizeof(int),1,fp);
    int len_name_of_graph;
    read = fread(&len_name_of_graph,sizeof(int),1,fp);
    char name_of_graph_against_which_was_cleaned[len_name_of_graph];
    read = fread(name_of_graph_against_which_was_cleaned,sizeof(char),len_name_of_graph, fp);
 }

  read = fread(magic_number,sizeof(char),6,fp);
  printf("%s\n", magic_number);


  /**
    Finally read the list of nodes in the graph and write out the kmers to a file as binary
  **/
  FILE *kfp = fopen("kmers", "w");
  uint32_t color_acc;

  int index = 0;
  int coverage[number_of_colours]; // not used currently
  while (1) {
    uint64_t kmer;
    char individual_edges_reading_from_binary[number_of_colours]; // bit field for which edges enter and leave
    read = fread(&kmer,sizeof(uint64_t),1,fp);
    if (read <= 0)
      break;

    read = fread(&coverage, sizeof(int), number_of_colours, fp);
    read = fread(individual_edges_reading_from_binary, sizeof(char), number_of_colours, fp);

    char edge = 0;
    for (i=0; i<number_of_colours;i++) {
      edge |= individual_edges_reading_from_binary[i] & 0xF;
    }
    // A (0) -> 0001, C (1) -> 0010, G (2) -> 0100, T (3) -> 1000
    for (i=0; i< 4; i++) {
      char mask = 1 << i;
      if (edge & mask) {
	// this only works with k < 32
	uint64_t k_plus_1 = kmer | i << (kmer_size * 2);
	fwrite(&k_plus_1, sizeof(uint64_t),1,kfp);
	index++;
	// now write out whether each color has this kmer edge
	color_acc = 0;
	for (j=0; j<number_of_colours;j++) {
	  if (individual_edges_reading_from_binary[j] & mask)
	    color_acc |= 1 << j % 32;
	  if ((j > 0 && j % 32 == 0) || j == number_of_colours -1) {
	    // write out bits when we have filled accumulator
	    fwrite(&color_acc, sizeof(uint32_t), 1,  kfp);
	    color_acc = 0;
	  }
	}

	/**
	   Write out the first 10 kmers as ascii
	**/
	if (index < 10) {
	  char bases[] = {'A','C','G','T'};
	  for (j=(kmer_size)*2; j>=0; j-=2) {
	    int val = (k_plus_1 & 3LL << j) >> j;
	    printf("%c", bases[val]);
	  }
	  printf("\n");
	}
      }
    }
  }
  fclose(kfp);
  printf("%d\n", index);
}
