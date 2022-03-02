// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

/**
 * Compile: cc -O2 -luuid parse.c -o parse
 * Usage: ./parse 7500 a.bin b.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include <assert.h>

int cmpuint64t(const void* u1, const void* u2) {
  const uint64_t *i1 = u1;
  const uint64_t *i2 = u2;
  return *i1 < *i2 ? -1 : 1;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s <count> <skip> <filepath>+\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  FILE *r, *w;
  uint64_t taglen;
  char tag[255], filename[255];
  uuid_t uuid;
  char uuidstr[37];
  uint64_t meascount;
  uint64_t meas[25*60*90]; /* can contain up to ninety minutes of meas */
  uint64_t target = atoi(argv[1]); /* subset of measures to consider */

  sprintf(filename, "%d.csv", target);
  if ((w = fopen(filename, "w")) == NULL) goto iofail;

  for (int i = 3; i < argc; i++) {
    printf("selected %s\n", argv[i]);
    if ((r = fopen(argv[i], "r")) == NULL) goto iofail;
    if (fread(&taglen, sizeof(taglen), 1, r) != 1) goto iofail;
    assert(taglen < 255);
    tag[taglen] = 0;
    if (fread(&tag, sizeof(char), taglen, r) != taglen) goto iofail;
    printf("tag: %s\n", tag);
    if (fread(uuid, sizeof(uuid_t), 1, r) != 1) goto iofail;
    uuid_unparse(uuid, uuidstr);
    printf("uuid: %s\n", uuidstr);
    if (fread(&meascount, sizeof(meascount), 1, r) != 1) goto iofail;
    assert(meascount == 135000);
    if (fread(meas, sizeof(meas[0]), meascount, r) != meascount) goto iofail;
    printf("read %d values\n", meascount);

    uint64_t* real_log = meas;
    uint64_t real_log_size = meascount;

    // cut beginning
    int i = atoi(argv[2]);
    while ((real_log[0] == 0 || i-- > 0) && real_log_size > 0) {
      real_log = &(real_log[1]);
      real_log_size--;
    }
    printf("cutted %lu values at beginning\n", meascount - real_log_size);
    if(real_log_size < target) goto file_done;

    uint64_t missmeas = 0;
    for (int j = 0; j < target; j++) {
      if (real_log[j] == 0) missmeas++;
    }
    uint64_t targetmeas = target - missmeas;

    // AVERAGE
    double avg = 0;
    for (int j = 0; j < target; j++) {
      if (real_log[j] == 0) continue;
      avg += ((double) real_log[j]) / ((double) targetmeas);
    }

    // DISTRIBUTION
    qsort (real_log, target, sizeof(uint64_t), cmpuint64t);
    uint64_t min = real_log[missmeas];
    uint64_t max = real_log[target-1];
    uint64_t med = real_log[(int)(0.50 * targetmeas) - 1 + missmeas];
    uint64_t q25 = real_log[(int)(0.25 * targetmeas) - 1 + missmeas];
    uint64_t q75 = real_log[(int)(0.75 * targetmeas) - 1 + missmeas];
    uint64_t q99 = real_log[(int)(0.99 * targetmeas) - 1 + missmeas];


    if (fprintf(w, "%s,%s,count,%lu\n", tag, uuidstr, targetmeas) < 0) goto iofail;
    if (fprintf(w,  "%s,%s,avg,%f\n",  tag, uuidstr, avg) < 0) goto iofail;
    if (fprintf(w,  "%s,%s,min,%lu\n", tag, uuidstr, min) < 0) goto iofail;
    if (fprintf(w,  "%s,%s,max,%lu\n", tag, uuidstr, max) < 0) goto iofail;
    if (fprintf(w,  "%s,%s,med,%lu\n", tag, uuidstr, med) < 0) goto iofail;
    if (fprintf(w,  "%s,%s,q25,%lu\n", tag, uuidstr, q25) < 0) goto iofail;
    if (fprintf(w,  "%s,%s,q75,%lu\n", tag, uuidstr, q75) < 0) goto iofail;
    if (fprintf(w,  "%s,%s,q99,%lu\n", tag, uuidstr, q99) < 0) goto iofail;

file_done:
    if(fclose(r) != 0) goto iofail;
  }


  if(fclose(w) != 0) goto iofail;
  printf("success\n");
  return EXIT_SUCCESS;

iofail:
  perror("an IO failure occurred"); 
  return EXIT_FAILURE;
}
