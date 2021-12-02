#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h> //for kmalloc
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#define MAX_NUM_OF_BIG_NODE 10


//define

struct small_node{
	struct list_head head;
	struct big_node *parent;
	int data;
	int idx;
};

struct big_node{
	struct list_head head;
	struct small_node *child;
	
};

struct tiered_list_head {
	struct list_head small_head;
	struct list_head big_head;
};



//initiate
struct tiered_list_head my_list;
struct list_head * p;

//function
void tiered_list_init(struct tiered_list_head * head){
	INIT_LIST_HEAD(&head->small_head);
	INIT_LIST_HEAD(&head->big_head);
}


void list_add_head(struct small_node *new,struct tiered_list_head* head){
	
	
	//case : first insert
	if(list_is_first(&my_list.small_head,&my_list.small_head)){	
		new->idx=1;	//assign index
		struct big_node *new_big=kmalloc(sizeof(struct big_node),GFP_KERNEL);	//make new big_node
		new_big->child=new;	//assign child to big_node
		new->parent=new_big;	//assign parent to small_node
		list_add(&new_big->head,&head->big_head);
		
		
	}
	
	// case : otherwise
	else{
		struct small_node *first_node;
		first_node=list_first_entry(&head->small_head,struct small_node,head);
		
		//first_node's idx==1
		if (first_node->idx==1){
			new->idx=MAX_NUM_OF_BIG_NODE;
			struct big_node *new_big=kmalloc(sizeof(struct big_node),GFP_KERNEL);
			new_big->child=new;
			new->parent=new_big;
			list_add(&new_big->head,&head->big_head);	//add new big_node
		}
		//else
		else{
			new->idx=first_node->idx-1;
		}
		
	}
	
	list_add(&new->head,&head->small_head);
	
}


void list_delete_tail(struct tiered_list_head* head){
	
	struct small_node *last_node;
	last_node=list_last_entry(&head->small_head,struct small_node,head);
	
	//case :have to delete big_node too
	if(last_node->idx==1){
		struct big_node *last_big_node;
		last_big_node=last_node->parent;
		
		list_del(&last_big_node->head);
		kfree(last_big_node);
	}
	
	list_del(&last_node->head);
	kfree(last_node);
	
	
	
}
int test1(void){

	int i;
	struct small_node *current_node;
	for (i=0;i<31;i++){
	
		struct small_node *new=kmalloc(sizeof(struct small_node),GFP_KERNEL);
		new->data=i;
		list_add_head(new,&my_list);
	}
	
	list_for_each(p,&my_list.small_head){
		current_node=list_entry(p,struct small_node,head);
		printk("(data,idx):(%d,%d)\n",current_node->data,current_node->idx);
		
	}
	int count_before_delete=0;
	list_for_each(p,&my_list.big_head){
		count_before_delete=count_before_delete+1;
		
	}
	printk("big head count before delete:%d\n",count_before_delete);
	
	
	printk("___________________________________________________________________");
	
	list_delete_tail(&my_list);
	list_for_each(p,&my_list.small_head){
		current_node=list_entry(p,struct small_node,head);
		printk("(data,idx):(%d,%d)\n",current_node->data,current_node->idx);
		
	}
	int count_before_after=0;
	list_for_each(p,&my_list.big_head){
		count_before_after=count_before_after+1;
		
	}
	printk("big head count before after:%d\n",count_before_after);
}


int __init advanced_linked_list_module_init(void){
	printk("advanced_linked_list init\n");
	
	tiered_list_init(&my_list);
	test1();

	

	return 0;
}

void __exit advanced_linked_list_cleanup(void){

	printk("advanced_linked_list bye\n");
}

module_init(advanced_linked_list_module_init);
module_exit(advanced_linked_list_cleanup);

MODULE_LICENSE("GPL");
