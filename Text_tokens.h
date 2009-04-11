 /*
  * Unitex
  *
  * Copyright (C) 2001-2009 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * 
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
  *
  */

#ifndef Text_tokensH
#define Text_tokensH


#include "List_int.h"
#include "String_hash.h"
#include "Unicode.h"
#include "Alphabet.h"
#include "DELA.h"


/*
 * This is the structure that holds 
 *  - The strings that each token represent (unichar **token)
 *  - The biggest token id                  (int N)
 *  - The token id of the sentence marker   (int SENTENCE_MARKER)
 *  - The token id of the stop marker       (int STOP_MARKER)
 *  - The token id of the space             (int SPACE)
 */

struct text_tokens {
   unichar** token;
   int N;
   int SENTENCE_MARKER;
   int STOP_MARKER;
   int SPACE;
};


struct text_tokens* load_text_tokens(char*);
struct string_hash* load_text_tokens_hash(char*,int*);
struct string_hash* load_text_tokens_hash(char*,int*,int*,int*);
void free_text_tokens(struct text_tokens*);
struct list_int* get_token_list_for_sequence(unichar*,Alphabet*,struct string_hash*);
int get_token_number(unichar*,struct text_tokens*);
int is_a_digit_token(unichar* s);
void extract_semantic_codes_from_tokens(struct string_hash*,struct string_hash*);
void extract_semantic_codes_from_morpho_dics(struct INF_codes**,int,struct string_hash*);

//---------------------------------------------------------------------------
#endif


