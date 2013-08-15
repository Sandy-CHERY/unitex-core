/*
 * Unitex
 *
 * Copyright (C) 2001-2013 Université Paris-Est Marne-la-Vallée <unitex@univ-mlv.fr>
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

#include "List_int.h"
#include "LocatePattern.h"
#include "OptimizedFst2.h"
#include "LocateConstants.h"
#include "Error.h"
#include "BitMasks.h"
#include "String_hash.h"
#include "Contexts.h"

#ifndef HAS_UNITEX_NAMESPACE
#define HAS_UNITEX_NAMESPACE 1
#endif

namespace unitex {

/**
 * Allocates, initializes and returns a new optimized graph call.
 */
struct opt_graph_call* new_opt_graph_call(int graph_number,Abstract_allocator prv_alloc) {
struct opt_graph_call* g;
g=(struct opt_graph_call*)malloc_cb(sizeof(struct opt_graph_call),prv_alloc);
if (g==NULL) {
   fatal_alloc_error("new_opt_graph_call");
}
g->graph_number=graph_number;
g->transition=NULL;
g->next=NULL;
return g;
}


/**
 * Frees the whole memory associate to the given graph call list.
 */
void free_opt_graph_call(struct opt_graph_call* list,Abstract_allocator prv_alloc) {
struct opt_graph_call* tmp;
while (list!=NULL) {
   free_Transition_list(list->transition,prv_alloc);
   tmp=list;
   list=list->next;
   free_cb(tmp,prv_alloc);
}
}


/**
 * This function adds the given graph call to given graph call list.
 */
void add_graph_call(Transition* transition,struct opt_graph_call** graph_calls,Abstract_allocator prv_alloc) {
int graph_number=-(transition->tag_number);
struct opt_graph_call* ptr=*graph_calls;
/* We look for a graph call for the same graph number */
while (ptr!=NULL && ptr->graph_number!=graph_number) {
   ptr=ptr->next;
}
if (ptr==NULL) {
   /* If we have found none, we create one */
   ptr=new_opt_graph_call(graph_number,prv_alloc);
   ptr->next=*graph_calls;
   *graph_calls=ptr;
}
/* Then, we add the transition to the graph call */
add_transition_if_not_present(&(ptr->transition),transition->tag_number,transition->state_number,prv_alloc);
}


/**
 * Allocates, initializes and returns a new opt_pattern.
 */
struct opt_pattern* new_opt_pattern(int pattern_number,int negation,Abstract_allocator prv_alloc) {
struct opt_pattern* p;
p=(struct opt_pattern*)malloc_cb(sizeof(struct opt_pattern),prv_alloc);
if (p==NULL) {
   fatal_alloc_error("new_opt_pattern");
}
p->pattern_number=pattern_number;
p->negation=(char)negation;
p->transition=NULL;
p->next=NULL;
return p;
}


/**
 * Frees the whole memory associated to the given optimized pattern list.
 */
void free_opt_pattern(struct opt_pattern* list,Abstract_allocator prv_alloc) {
struct opt_pattern* tmp;
while (list!=NULL) {
   free_Transition_list(list->transition,prv_alloc);
   tmp=list;
   list=list->next;
   free_cb(tmp,prv_alloc);
}
}


/**
 * This function adds the given pattern number to the given pattern list.
 */
void add_pattern(int pattern_number,Transition* transition,struct opt_pattern** pattern_list,int negation,Abstract_allocator prv_alloc) {
struct opt_pattern* ptr=*pattern_list;
/* We look for a pattern with the same properties */
while (ptr!=NULL && !(ptr->pattern_number==pattern_number && ptr->negation==negation)) {
   ptr=ptr->next;
}
if (ptr==NULL) {
   /* If we have not found an equivalent pattern, we create one */
   ptr=new_opt_pattern(pattern_number,negation,prv_alloc);
   ptr->next=*pattern_list;
   *pattern_list=ptr;
}
/* Finally, we add the transition to the pattern */
add_transition_if_not_present(&(ptr->transition),transition->tag_number,transition->state_number,prv_alloc);
}


/**
 * Allocates, initializes and returns a new optimized token.
 */
struct opt_token* new_opt_token(int token_number,Abstract_allocator prv_alloc) {
struct opt_token* t;
t=(struct opt_token*)malloc_cb(sizeof(struct opt_token),prv_alloc);
if (t==NULL) {
   fatal_alloc_error("new_opt_token");
}
t->token_number=token_number;
t->transition=NULL;
t->next=NULL;
return t;
}


/**
 * Frees all the memory associated to the given token list.
 */
void free_opt_token(struct opt_token* list,Abstract_allocator prv_alloc) {
struct opt_token* tmp;
while (list!=NULL) {
   free_Transition_list(list->transition,prv_alloc);
   tmp=list;
   list=list->next;
   free_cb(tmp,prv_alloc);
}
}


/**
 * This function add the given token in the given 'token_list', if not already present.
 * '*number_of_tokens' is increased of 1 if the token was not present.
 *
 * Note that this function must perform a sorted insert, since the resulting
 * list will be supposed to be sorted at the time of converting it into an array
 * in 'token_list_2_token_array'.
 */
void add_token(int token_number,Transition* transition,struct opt_token** token_list,
               int *number_of_tokens,Abstract_allocator prv_alloc) {
struct opt_token* ptr;
if (*token_list==NULL) {
   /* If the list is empty, we add the token */
   (*token_list)=new_opt_token(token_number,prv_alloc);
   add_transition_if_not_present(&((*token_list)->transition),transition->tag_number,transition->state_number,prv_alloc);
   (*number_of_tokens)++;
   return;
}
/* If we must insert before the head of the list */
if (token_number<(*token_list)->token_number) {
   /* If the list is empty, we add the token */
   ptr=new_opt_token(token_number,prv_alloc);
   add_transition_if_not_present(&(ptr->transition),transition->tag_number,transition->state_number,prv_alloc);
   ptr->next=(*token_list);
   (*token_list)=ptr;
   (*number_of_tokens)++;
   return;
}
/* If we must update the head of the list */
if (token_number==(*token_list)->token_number) {
   add_transition_if_not_present(&((*token_list)->transition),transition->tag_number,transition->state_number,prv_alloc);
   return;
}
/* Otherwise, we look for the exact token, or the last token that is lower than
 * the one we look for */
ptr=(*token_list);
while (ptr->next!=NULL && (ptr->next->token_number<token_number)) {
   ptr=ptr->next;
}
if (ptr->next==NULL || ptr->next->token_number>token_number) {
   /* If we are at the end of list or before a greater token, then we must create the
    * token and insert it after 'ptr'. */
   struct opt_token* tmp=new_opt_token(token_number,prv_alloc);
   add_transition_if_not_present(&(tmp->transition),transition->tag_number,transition->state_number,prv_alloc);
   tmp->next=ptr->next;
   ptr->next=tmp;
   (*number_of_tokens)++;
   return;
}
/* Otherwise, 'ptr->next' points on the token we look for */
add_transition_if_not_present(&(ptr->next->transition),transition->tag_number,transition->state_number,prv_alloc);
}


/**
 * This function adds all the tokens contained in the given 'token_list' into
 * 'list' that contains all the tokens matched by an optimized state.
 * '*number_of_tokens' is updated.
 */
void add_token_list(struct list_int* token_list,Transition* transition,
                    struct opt_token** list,int *number_of_tokens,Abstract_allocator prv_alloc) {
while (token_list!=NULL) {
   add_token(token_list->n,transition,list,number_of_tokens,prv_alloc);
   token_list=token_list->next;
}
}


/**
 * Allocates, initializes and returns a new optimized meta.
 */
struct opt_meta* new_opt_meta(enum meta_symbol meta,int negation,Abstract_allocator prv_alloc) {
struct opt_meta* m;
m=(struct opt_meta*)malloc_cb(sizeof(struct opt_meta),prv_alloc);
if (m==NULL) {
   fatal_alloc_error("new_opt_meta");
}
m->meta=meta;
m->negation=(char)negation;
m->transition=NULL;
m->next=NULL;
return m;
}


/**
 * Frees all the memory associated to the given meta list.
 */
void free_opt_meta(struct opt_meta* list,Abstract_allocator prv_alloc) {
struct opt_meta* tmp;
while (list!=NULL) {
   free_Transition_list(list->transition,prv_alloc);
   tmp=list;
   list=list->next;
   free_cb(tmp,prv_alloc);
}
}


/**
 * This function adds the given meta to the given meta list.
 */
void add_meta(enum meta_symbol meta,Transition* transition,struct opt_meta** meta_list,int negation,Abstract_allocator prv_alloc) {
struct opt_meta* ptr=*meta_list;
/* We look for a meta with the same properties */
while (ptr!=NULL && !(ptr->meta==meta && ptr->negation==negation)) {
   ptr=ptr->next;
}
if (ptr==NULL) {
   /* If we have found none, we create one */
   ptr=new_opt_meta(meta,negation,prv_alloc);
   ptr->next=*meta_list;
   *meta_list=ptr;
}
/* Then, we add the transition to the meta */
add_transition_if_not_present(&(ptr->transition),transition->tag_number,transition->state_number,prv_alloc);
}


/**
 * Allocates, initializes and returns a new optimized variable.
 */
struct opt_variable* new_opt_variable(int variable_number,Transition* transition,
										Abstract_allocator prv_alloc) {
struct opt_variable* v;
v=(struct opt_variable*)malloc_cb(sizeof(struct opt_variable),prv_alloc);
if (v==NULL) {
   fatal_alloc_error("new_opt_variable");
}
v->variable_number=variable_number;
v->transition=NULL;
add_transition_if_not_present(&(v->transition),transition->tag_number,transition->state_number,prv_alloc);
v->next=NULL;
return v;
}


/**
 * Frees all the memory associated to the given variable list.
 */
void free_opt_variable(struct opt_variable* list,Abstract_allocator prv_alloc) {
struct opt_variable* tmp;
while (list!=NULL) {
   free_Transition_list(list->transition,prv_alloc);
   tmp=list;
   list=list->next;
   free_cb(tmp,prv_alloc);
}
}


/**
 * This function adds the given variable to the given variable list.
 * No tests is done to check if there is already a transition with the
 * given variable, because it cannot happen if the grammar is deterministic.
 */
void add_input_variable(Variables* var,unichar* variable,Transition* transition,
		struct opt_variable** variable_list,Abstract_allocator prv_alloc) {
int n=get_value_index(variable,var->variable_index,DONT_INSERT);
struct opt_variable* v=new_opt_variable(n,transition,prv_alloc);
v->next=(*variable_list);
(*variable_list)=v;
}


/**
 * This function adds the given variable to the given variable list.
 * No tests is done to check if there is already a transition with the
 * given variable, because it cannot happen if the grammar is deterministic.
 */
void add_output_variable(OutputVariables* var,unichar* variable,Transition* transition,
		struct opt_variable** variable_list,Abstract_allocator prv_alloc) {
int n=get_value_index(variable,var->variable_index,DONT_INSERT);
struct opt_variable* v=new_opt_variable(n,transition,prv_alloc);
v->next=(*variable_list);
(*variable_list)=v;
}


/**
 * Adds a positive context to the given state. As a side effect, this function looks for all the closing
 * context marks reachable from this positive context mark and stores them into
 * 'reacheable_states_from_positive_context'. If there is no reachable context
 * end mark, the function emit an error message and ignores this "$[" transition.
 */
void add_positive_context(Fst2* fst2,OptimizedFst2State state,Transition* transition,Abstract_allocator prv_alloc) {
add_positive_context(fst2,&(state->contexts),transition,prv_alloc);
}


/**
 * Adds a negative context to the given state. As a side effect, this function looks for all the closing
 * context marks reachable from this negative context mark and stores them into
 * 'reacheable_states_from_negative_context'. If there is no reachable context
 * end mark, the function emit an error message and ignores this "$![" transition.
 */
void add_negative_context(Fst2* fst2,OptimizedFst2State state,Transition* transition,Abstract_allocator prv_alloc) {
add_negative_context(fst2,&(state->contexts),transition,prv_alloc);
}


/**
 * Adds a context end to the given state or raises a fatal error if
 * there is already one, because it would mean that the fst2 is not
 * deterministic.
 */
void add_end_context(OptimizedFst2State state,Transition* transition,Abstract_allocator prv_alloc) {
if (state->contexts==NULL) {
   state->contexts=new_opt_contexts(prv_alloc);
}
if (state->contexts->end_mark!=NULL) {
   fatal_error("Duplicate end context mark\n");
}
state->contexts->end_mark=new_Transition(transition->tag_number,transition->state_number,prv_alloc);
}


/**
 * This function optimizes the given transition.
 */
void optimize_transition(Variables* v,OutputVariables* output,Fst2* fst2,Transition* transition,
						OptimizedFst2State state,Fst2Tag* tags,Abstract_allocator prv_alloc) {
if (transition->tag_number<0) {
   /* If the transition is a graph call */
   add_graph_call(transition,&(state->graph_calls),prv_alloc);
   return;
}
Fst2Tag tag=tags[transition->tag_number];
if (tag==NULL) {
   fatal_error("NULL transition tag in optimize_transition\n");
}
int negation=is_bit_mask_set(tag->control,NEGATION_TAG_BIT_MASK);
/* First, we look if there is a compound pattern associated to this tag */
if (tag->compound_pattern!=NO_COMPOUND_PATTERN) {
   add_pattern(tag->compound_pattern,transition,&(state->compound_patterns),negation,prv_alloc);
}
/* Then, we look the possible kind of transitions */
switch (tag->type) {
   case TOKEN_LIST_TAG: add_token_list(tag->matching_tokens,transition,&(state->token_list),&(state->number_of_tokens),prv_alloc);
                        return;
   case PATTERN_NUMBER_TAG: add_pattern(tag->pattern_number,transition,&(state->patterns),negation,prv_alloc);
                            return;
   case META_TAG: add_meta(tag->meta,transition,&(state->metas),negation,prv_alloc);
                           return;
   case BEGIN_VAR_TAG: add_input_variable(v,tag->variable,transition,&(state->input_variable_starts),prv_alloc);
                       return;
   case END_VAR_TAG: add_input_variable(v,tag->variable,transition,&(state->input_variable_ends),prv_alloc);
                     return;
   case BEGIN_OUTPUT_VAR_TAG: add_output_variable(output,tag->variable,transition,&(state->output_variable_starts),prv_alloc);
                       return;
   case END_OUTPUT_VAR_TAG: add_output_variable(output,tag->variable,transition,&(state->output_variable_ends),prv_alloc);
                     return;
   case BEGIN_POSITIVE_CONTEXT_TAG: add_positive_context(fst2,state,transition,prv_alloc); return;
   case BEGIN_NEGATIVE_CONTEXT_TAG: add_negative_context(fst2,state,transition,prv_alloc); return;
   case END_CONTEXT_TAG: add_end_context(state,transition,prv_alloc); return;
   case LEFT_CONTEXT_TAG: add_meta(META_LEFT_CONTEXT,transition,&(state->metas),0,prv_alloc); return;
   case BEGIN_MORPHO_TAG: add_meta(META_BEGIN_MORPHO,transition,&(state->metas),0,prv_alloc); return;
   case END_MORPHO_TAG: add_meta(META_END_MORPHO,transition,&(state->metas),0,prv_alloc); return;
   case TEXT_START_TAG: add_meta(META_TEXT_START,transition,&(state->metas),0,prv_alloc); return;
   case TEXT_END_TAG: add_meta(META_TEXT_END,transition,&(state->metas),0,prv_alloc); return;
   default: fatal_error("Unexpected transition tag type in optimize_transition\n");
}
}


/**
 * This function builds an array containing the token numbers stored
 * in the token list of the given state. As the token list is supposed
 * to be sorted, the array will be sorted. The function frees the token
 * list.
 */
void token_list_2_token_array(OptimizedFst2State state,Abstract_allocator prv_alloc) {
int i;
struct opt_token* l;
struct opt_token* tmp;
if (state->number_of_tokens==0) {
   /* Nothing to do if there is no token in the list */
   return;
}
state->tokens=(int*)malloc_cb(sizeof(int)*state->number_of_tokens,prv_alloc);
if (state->tokens==NULL) {
   fatal_alloc_error("token_list_2_token_array");
}
state->token_transitions=(Transition**)malloc_cb(sizeof(Transition*)*state->number_of_tokens,prv_alloc);
if (state->token_transitions==NULL) {
   fatal_alloc_error("token_list_2_token_array");
}
i=0;
l=state->token_list;
while (l!=NULL) {
   state->tokens[i]=l->token_number;
   state->token_transitions[i]=l->transition;
   i++;
   tmp=l;
   l=l->next;
   /* We must NOT free 'tmp->transition' since it is referenced now
    * in 'state->token_transitions[i]' */
   free_cb(tmp,prv_alloc);
}
if (i!=state->number_of_tokens) {
   fatal_error("Internal error in token_list_2_token_array\n");
}
state->token_list=NULL;
}


/**
 * Allocates, initializes and returns a new optimized state.
 */
OptimizedFst2State new_optimized_state(Abstract_allocator prv_alloc) {
OptimizedFst2State state=(OptimizedFst2State)malloc_cb(sizeof(struct optimizedFst2State),prv_alloc);
if (state==NULL) {
   fatal_alloc_error("new_optimized_state");
}
state->control=0;
state->graph_calls=NULL;
state->metas=NULL;
state->patterns=NULL;
state->compound_patterns=NULL;
state->token_list=NULL;
state->tokens=NULL;
state->token_transitions=NULL;
state->number_of_tokens=0;
state->input_variable_starts=NULL;
state->input_variable_ends=NULL;
state->output_variable_starts=NULL;
state->output_variable_ends=NULL;
state->contexts=NULL;
return state;
}


/**
 * Frees the whole memory associated to the given optimized state.
 */
void free_optimized_state(OptimizedFst2State state,Abstract_allocator prv_alloc) {
if (state==NULL) return;
free_opt_graph_call(state->graph_calls,prv_alloc);
free_opt_meta(state->metas,prv_alloc);
free_opt_pattern(state->patterns,prv_alloc);
free_opt_pattern(state->compound_patterns,prv_alloc);
free_opt_token(state->token_list,prv_alloc);
free_opt_variable(state->input_variable_starts,prv_alloc);
free_opt_variable(state->input_variable_ends,prv_alloc);
free_opt_variable(state->output_variable_starts,prv_alloc);
free_opt_variable(state->output_variable_ends,prv_alloc);
free_opt_contexts(state->contexts,prv_alloc);
if (state->tokens!=NULL) free_cb(state->tokens,prv_alloc);
if (state->token_transitions!=NULL) {
   for (int i=0;i<state->number_of_tokens;i++) {
      free_Transition_list(state->token_transitions[i],prv_alloc);
   }
   free_cb(state->token_transitions,prv_alloc);
}
free_cb(state,prv_alloc);
}


/**
 * This function looks all the transitions that outgo from the given state
 * and returns an equivalent optimized state, or NULL if the given state
 * was NULL.
 */
OptimizedFst2State optimize_state(Variables* v,OutputVariables* output,Fst2* fst2,Fst2State state,
									Fst2Tag* tags,Abstract_allocator prv_alloc) {
if (state==NULL) return NULL;
OptimizedFst2State new_state=new_optimized_state(prv_alloc);
new_state->control=state->control;
Transition* ptr=state->transitions;
while (ptr!=NULL) {
   optimize_transition(v,output,fst2,ptr,new_state,tags,prv_alloc);
   ptr=ptr->next;
}
return new_state;
}


/**
 * This function must be updated if the implementation of 'is_final_state' changes
 * in Fst2.cpp
 */
static int is_final_state(OptimizedFst2State e) {
if (e==NULL) {
   fatal_error("NULL error in is_final_state\n");
}
return e->control&1;
}


/**
 * Returns 1 if the state is not final and has no outgoing transition; 0 otherwise.
 */
static int is_useless_state(OptimizedFst2State s) {
return !is_final_state(s)
		&& s->graph_calls==NULL
		&& s->metas==NULL
		&& s->patterns==NULL
		&& s->compound_patterns==NULL
		&& s->number_of_tokens==0
		&& s->input_variable_starts==NULL
		&& s->input_variable_ends==NULL
		&& s->output_variable_starts==NULL
		&& s->output_variable_ends==NULL
		&& (s->contexts==NULL || (s->contexts->non_NULL_positive_transitions==0 && s->contexts->size_negative==0 && s->contexts->end_mark==NULL));
}


/**
 * Returns a positive value if the given graph is not useless, i.e.
 * its initial state is final and/or it has transitions; 0 otherwise.
 */
static int empty_graph(int graph,OptimizedFst2State* optimized_states,Fst2* fst2) {
OptimizedFst2State initial=optimized_states[fst2->initial_states[graph]];
return is_useless_state(initial);
}


/**
 * Removes the useless graph calls, i.e. calls to an empty graph or calls
 * to a state that has no outgoing transitions.
 */
static int remove_useless_graph_calls(struct opt_graph_call* *l,OptimizedFst2State* optimized_states,Fst2* fst2,
		Abstract_allocator prv_alloc) {
int removed=0;
struct opt_graph_call* *tmp=l;
while ((*tmp)!=NULL) {
	if (empty_graph((*tmp)->graph_number,optimized_states,fst2)
			|| is_useless_state(optimized_states[(*tmp)->transition->state_number])) {
		/* We remove the graph call */
		removed=1;
		struct opt_graph_call* current=(*tmp);
		(*tmp)=current->next;
		current->next=NULL;
		free_opt_graph_call(current,prv_alloc);
	} else {
		tmp=&((*tmp)->next);
	}
}
return removed;
}

/**
 * We return a copy of the list, without the transitions that pointed
 * to a useless state.
 */
static Transition* filter_transitions(Transition* l,OptimizedFst2State* optimized_states,Abstract_allocator prv_alloc) {
if (l==NULL) return NULL;
l->next=filter_transitions(l->next,optimized_states,prv_alloc);
if (is_useless_state(optimized_states[l->state_number])) {
	Transition* tmp=l->next;
	l->next=NULL;
	free_Transition_list(l,prv_alloc);
	return tmp;
}
return l;
}


/**
 * Removes the useless metas, i.e. metas to a state that has no outgoing transitions.
 */
static int remove_useless_metas(struct opt_meta* *l,OptimizedFst2State* optimized_states,
		Abstract_allocator prv_alloc) {
int removed=0;
struct opt_meta* *tmp=l;
while ((*tmp)!=NULL) {
	(*tmp)->transition=filter_transitions((*tmp)->transition,optimized_states,prv_alloc);
	if ((*tmp)->transition==NULL) {
		/* We remove the meta */
		removed=1;
		struct opt_meta* current=(*tmp);
		(*tmp)=current->next;
		current->next=NULL;
		free_opt_meta(current,prv_alloc);
	} else {
		tmp=&((*tmp)->next);
	}
}
return removed;
}


/**
 * Removes the useless patterns, i.e. patterns to a state that has no outgoing transitions.
 */
static int remove_useless_patterns(struct opt_pattern* *l,OptimizedFst2State* optimized_states,
		Abstract_allocator prv_alloc) {
int removed=0;
struct opt_pattern* *tmp=l;
while ((*tmp)!=NULL) {
	(*tmp)->transition=filter_transitions((*tmp)->transition,optimized_states,prv_alloc);
	if ((*tmp)->transition==NULL) {
		/* We remove the pattern */
		removed=1;
		struct opt_pattern* current=(*tmp);
		(*tmp)=current->next;
		current->next=NULL;
		free_opt_pattern(current,prv_alloc);
	} else {
		tmp=&((*tmp)->next);
	}
}
return removed;
}


/**
 * Removes the useless variables, i.e. variables to a state that has no outgoing transitions.
 */
static int remove_useless_variables(struct opt_variable* *l,OptimizedFst2State* optimized_states,
		Abstract_allocator prv_alloc) {
int removed=0;
struct opt_variable* *tmp=l;
while ((*tmp)!=NULL) {
	(*tmp)->transition=filter_transitions((*tmp)->transition,optimized_states,prv_alloc);
	if ((*tmp)->transition==NULL) {
		/* We remove the variable */
		removed=1;
		struct opt_variable* current=(*tmp);
		(*tmp)=current->next;
		current->next=NULL;
		free_opt_variable(current,prv_alloc);
	} else {
		tmp=&((*tmp)->next);
	}
}
return removed;
}


/**
 * Removes the useless contexts, i.e. contexts to a state that has no outgoing transitions.
 */
static int remove_useless_contexts(struct opt_contexts* c,OptimizedFst2State* optimized_states,
		Abstract_allocator prv_alloc) {
if (c==NULL) return 0;
int removed=0;
/* We filter the positive contexts */
for (int i=0;i<c->size_positive;i+=2) {
	/* We have two transitions to consider: the one outgoing from the context mark, and
	 * the one pointing to the state after the end context mark. If either of those transitions
	 * points to a useless states, we can remove both.
	 */
	if (c->positive_mark[i]==NULL || c->positive_mark[i+1]==NULL) {
		continue;
	}
	int dest1=c->positive_mark[i]->state_number;
	int dest2=c->positive_mark[i+1]->state_number;
	if (is_useless_state(optimized_states[dest1]) || is_useless_state(optimized_states[dest2])) {
		free_Transition_list(c->positive_mark[i],prv_alloc);
		free_Transition_list(c->positive_mark[i+1],prv_alloc);
		c->positive_mark[i]=NULL;
		c->positive_mark[i+1]=NULL;
		c->non_NULL_positive_transitions-=2;
		removed=1;
	}
}
/* We don't filter the negative contexts, because inside a negative context,
 * a failure to match is significant. So, if we have a token 'foo' that cannot
 * match, we must not try to trim the graph too much by removing the
 * negative context mark or even modify its outgoing transitions.
 */

/* Finally, we filter the end context mark, if any */
if (c->end_mark!=NULL) {
	int dest=c->end_mark->state_number;
	if (is_useless_state(optimized_states[dest])) {
		free_Transition_list(c->end_mark);
		c->end_mark=NULL;
		removed=1;
	}
}
return removed;
}


/**
 * Removes the useless tokens, i.e. tokens to a state that has no outgoing transitions.
 */
static int remove_useless_tokens(OptimizedFst2State state,OptimizedFst2State* optimized_states,
		Abstract_allocator prv_alloc) {
int removed=0;
struct opt_token* *tmp=&(state->token_list);
while ((*tmp)!=NULL) {
	/* For each token, we have to filter its whole transition list */
	(*tmp)->transition=filter_transitions((*tmp)->transition,optimized_states,prv_alloc);
	if ((*tmp)->transition==NULL) {
		/* We remove the token if all its transitions have been removed */
		removed=1;
		state->number_of_tokens--;
		struct opt_token* current=(*tmp);
		(*tmp)=current->next;
		current->next=NULL;
		free_opt_token(current,prv_alloc);
	} else {
		tmp=&((*tmp)->next);
	}
}
return removed;
}


/**
 * Removes the useless transitions from s. Returns 0 if no transition was removed, a positive value
 * otherwise.
 */
static int remove_useless_transitions(OptimizedFst2State s,OptimizedFst2State* optimized_states,Fst2* fst2,
		Abstract_allocator prv_alloc) {
return remove_useless_graph_calls(&(s->graph_calls),optimized_states,fst2,prv_alloc)
		+ remove_useless_metas(&(s->metas),optimized_states,prv_alloc)
		+ remove_useless_patterns(&(s->patterns),optimized_states,prv_alloc)
		+ remove_useless_patterns(&(s->compound_patterns),optimized_states,prv_alloc)
		+ remove_useless_variables(&(s->input_variable_starts),optimized_states,prv_alloc)
		+ remove_useless_variables(&(s->input_variable_ends),optimized_states,prv_alloc)
        + remove_useless_variables(&(s->output_variable_starts),optimized_states,prv_alloc)
        + remove_useless_variables(&(s->output_variable_ends),optimized_states,prv_alloc)
        + remove_useless_contexts(s->contexts,optimized_states,prv_alloc)
        + remove_useless_tokens(s,optimized_states,prv_alloc);
}


/**
 * This function removes all useless transitions from the given graph. A useless
 * transitions is a transition to a state that has no transition.
 * Returns 1 the graph becomes empty; 0 otherwise.
 */
static int remove_useless_lexical_transitions(Fst2* fst2,int graph,OptimizedFst2State* optimized_states,
							Abstract_allocator prv_alloc) {
int initial=fst2->initial_states[graph];
int last=initial+fst2->number_of_states_per_graphs[graph]-1;
int cleaning_to_do=1;
if (empty_graph(graph,optimized_states,fst2)) {
	/* If the graph is already empty, there is nothing to report */
	return 0;
}
while (cleaning_to_do) {
	cleaning_to_do=0;
	for (int i=initial;i<=last;i++) {
		OptimizedFst2State s=optimized_states[i];
		int removed=remove_useless_transitions(s,optimized_states,fst2,prv_alloc);
		if (removed && !cleaning_to_do) {
			/* This state may now be useless */
			if (is_useless_state(s)) {
				/* If this is the case, we will have another round to do on this graph */
				cleaning_to_do=1;
			}
		}
	}
}
return empty_graph(graph,optimized_states,fst2);
}


/**
 * This function takes a fst2 and returns an array containing the corresponding
 * optimized states.
 */
OptimizedFst2State* build_optimized_fst2_states(Variables* v,OutputVariables* output,Fst2* fst2,Abstract_allocator prv_alloc) {
OptimizedFst2State* optimized_states=(OptimizedFst2State*)malloc_cb(fst2->number_of_states*sizeof(OptimizedFst2State),prv_alloc);
if (optimized_states==NULL) {
   fatal_alloc_error("build_optimized_fst2_states");
}
for (int i=0;i<fst2->number_of_states;i++) {
   optimized_states[i]=optimize_state(v,output,fst2,fst2->states[i],fst2->tags,prv_alloc);
}
int n_graphs_emptied;
do {
	n_graphs_emptied=0;
	for (int i=1;i<=fst2->number_of_graphs;i++) {
		n_graphs_emptied+=remove_useless_lexical_transitions(fst2,i,optimized_states,prv_alloc);
	}
} while (n_graphs_emptied!=0);
/* Finally, we convert token lists to sorted array suitable for binary search */
for (int i=0;i<fst2->number_of_states;i++) {
	token_list_2_token_array(optimized_states[i],prv_alloc);
}
return optimized_states;
}


/**
 * Frees the whole memory associated to the given optimized state array.
 */
void free_optimized_states(OptimizedFst2State* states,int size,Abstract_allocator prv_alloc) {
if (states==NULL) return;
for (int i=0;i<size;i++) {
   free_optimized_state(states[i],prv_alloc);
}
free_cb(states,prv_alloc);
}

} // namespace unitex
