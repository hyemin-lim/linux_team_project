#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>

struct big_node{
    struct list_head head;
    struct small_node *child;
}

struct small_node{
    struct list_head head;
    struct big_node *parent;
    int idx;
}

struct tiered_list_head {
    struct list_head small_head;
    struct list_head big_head;
}

void INIT_TIERED_LIST(struct tiered_list_head *new) {
    INIT_LIST_HEAD(&(new->small_head));
    INIT_LIST_HEAD(&(new->big_head));
}

struct tiered_list_head tiered_list;

void tiered_list_add_tail(struct small_node *new, struct tiered_list_head *head){
    //if list is empty
    if(list_empty(&(head->small_head))){
        new->idx = 0; //idx starts from zero
        struct big_node *new_big = kmalloc(sizeof(struct big_node), GFP_KERNEL); //create new big node
        new_big->child = new; //link big->small
        new->parent = new_big; //link small->big
        list_add_tail(&(new->head), &(head->small_head)); //add small node to the list
        list_add_tail(&(new_big->head), &(head->big_head)); //add big node to the list
    }
    else{
        struct small_node *last_entry = list_last_entry(&(head->small_head), struct small_node, head); //get the last node of the list
        int last_idx = last_entry->idx; //get the idx of the last node
        int new_idx = (last_idx + 1) % 10; // new idx is (last idx + 1) % 10 => idx range is 0~9
        new->idx = new_idx; //assign new idx

        if(new_idx == 0){ //if new idx is 0, big node needs to be linked before addition
            
            struct big_node *new_big = kmalloc(sizeof(struct big_node), GFP_KERNEL); //create new big node
            new_big->child = new; //link big->small
            new->parent = new_big; //link small->big
            list_add_tail(&(new->head), &(head->small_head)); //add small node to the list
            list_add_tail(&(new_big->head), &(head->big_head)); //add big node to the list
        }
        else{ //if new idx is 1~9, just add it to the tail
            list_add_tail(&(new->head), &(head->small_head));
        }

    }
    
}
void traverse(struct tiered_list_head *head){
    struct small_node *curr;
    list_for_each_entry(curr, &(head.small_head), head){
        printk("current value : %d\n", curr->idx);
    }
}

