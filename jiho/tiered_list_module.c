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
#define BILLION 1000000000


//define

unsigned long long total_time = 0;

unsigned long long calclock3(struct timespec *spclock, unsigned long long *total_time)
{
    long temp, temp_n;
    unsigned long long timedelay = 0;
    if (spclock[1].tv_nsec >= spclock[0].tv_nsec)
    {
        temp = spclock[1].tv_sec - spclock[0].tv_sec;
        temp_n = spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }
    else 
    {
        temp = spclock[1].tv_sec - spclock[0].tv_sec - 1;
        temp_n = BILLION + spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }

    __sync_fetch_and_add(total_time, timedelay);
  
    return timedelay;
}

struct timespec spclock[2];
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
void tiered_list_init(struct tiered_list_head * new){
	INIT_LIST_HEAD(&new->small_head);
	INIT_LIST_HEAD(&new->big_head);
}


void tiered_list_add(struct small_node *new,struct tiered_list_head* head){
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
		if (first_node->idx==2){
			new->idx=1;
			struct big_node *new_big=kmalloc(sizeof(struct big_node),GFP_KERNEL);
			new_big->child=new;
			new->parent=new_big;
			list_add(&new_big->head,&head->big_head);	//add new big_node
		}
		//else
		else{
			if (first_node->idx==1){
				new->idx=MAX_NUM_OF_BIG_NODE;
			}
			else{
				new->idx=first_node->idx-1;
			}
			
		}
	}
	
	list_add(&new->head,&head->small_head);
}
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
        int new_idx = (last_idx + 1) % MAX_NUM_OF_BIG_NODE; // new idx is (last idx + 1) % 10 => idx range is 0~9
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
void tiered_list_delete(struct tiered_list_head* head){
	
	struct small_node *first_node;
	first_node=list_first_entry(&head->small_head,struct small_node,head);
	
	//case :have to delete big_node too
	if(first_node->idx==1){
		struct big_node *first_big_node;
		first_big_node=first_node->parent;
		list_del(&first_big_node->head);
		kfree(first_big_node);

	}

	list_del(&first_node->head);
	kfree(first_node);
	
}
void tiered_list_delete_tail(struct tiered_list_head* head){
	
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
/*
struct small_node* get(int find, struct tiered_list_head* head){		// finding idx
	struct small_node *small;
	small=list_first_entry(&head->small_head,struct small_node,head);
	//printk("small node -> (data,idx):(%d,%d)\n",small->data,small->idx);
	
	int i = 1;
	if (find < MAX_NUM_OF_BIG_NODE){	// don't have to jump big_head.
		list_for_each(p,&my_list.small_head){
			small=list_entry(p,struct small_node,head);
			if (i == find){
				//printk("1(data,idx):(%d,%d)\n",small->data,small->idx);
				return small;
			}
			i++;
		}
	}
	else{
		i=0;
		struct big_node *big;
		big=list_first_entry(&head->big_head,struct big_node,head);
		printk("%d\n\n",MAX_NUM_OF_BIG_NODE-small->idx+1);
		int jump = (find-(MAX_NUM_OF_BIG_NODE-small->idx+1))/MAX_NUM_OF_BIG_NODE; // for big_head jump
		int small_step = (find - (MAX_NUM_OF_BIG_NODE-small->idx+1))%(MAX_NUM_OF_BIG_NODE); // for small_head traverse
		printk("%d\n\n",small_step);
		//printk("(jump -> %d), (small_step -> %d)", jump, small_step);
		
		list_for_each(p,&my_list.big_head){
			if (i==jump){	// end jumping
				break;
			}
			big=list_entry(p,struct big_node,head);
			small = big->child;
			//printk("2(data,idx):(%d,%d) %p\n",small->data, small->idx, small->head);
			i++;
		}
		
		i=1;
		list_for_each(p,&small->head){
			small=list_entry(p,struct small_node,head);
			if (i == small_step){
				//printk("3(data,idx):(%d,%d)\n",small->data,small->idx);
				return small;
			}
			i++;
		}
	}
	
}
*/
struct small_node* get(int find, struct tiered_list_head* head){		// finding idx
	struct small_node *small;
	small=list_first_entry(&head->small_head,struct small_node,head);
	//printk("small node -> (data,idx):(%d,%d)\n",small->data,small->idx);
	
	int i = 1;
	if (find < MAX_NUM_OF_BIG_NODE){	// don't have to jump big_head.
		list_for_each(p,&my_list.small_head){
			small=list_entry(p,struct small_node,head);
			if (i == find){
				//printk("1(data,idx):(%d,%d)\n",small->data,small->idx);
				return small;
			}
			i++;
		}
	}
	else{
		i=0;
		int small_step = 1;
		struct small_node *current_node;
		struct big_node *big;
		big=list_first_entry(&head->big_head,struct big_node,head);
		
		list_for_each(p,&my_list.small_head){
			current_node=list_entry(p,struct small_node,head);
			//printk("1(data,idx):(%d,%d), (bighead : %p)\n",current_node->data,current_node->idx, current_node->parent);
			if (current_node->parent != NULL){
				break;
			}
			small_step++;
		}
	
		int jump = (find - small_step) / MAX_NUM_OF_BIG_NODE;
		// int jump = find/MAX_NUM_OF_BIG_NODE; // for big_head jump
		// int small_step = find - (MAX_NUM_OF_BIG_NODE - small->idx) - 1; // for small_head traverse
		//printk("(jump -> %d), (small_step -> %d)", jump, small_step);
		
		list_for_each(p,&my_list.big_head){
			if (i==jump){	// end jumping
				break;
			}
			big=list_entry(p,struct big_node,head);
			small = big->child;
			//printk("2(data,idx):(%d,%d) %p\n",small->data, small->idx, small->head);
			i++;
		}
		
		i=1;
		list_for_each(p,&small->head){
			small=list_entry(p,struct small_node,head);
			if (i == small_step){
				//printk("3(data,idx):(%d,%d)\n",small->data,small->idx);
				return small;
			}
			i++;
		}
	}
	
}

void traverse(struct tiered_list_head *head){
    struct small_node *curr;
    list_for_each_entry(curr, &(head->small_head), head){
        printk("current value : %d\n", curr->idx);
    }
}
void test1(void){

	int i;
	struct small_node *current_node;
	for (i=0;i<1000;i++){
	
		struct small_node *new=kmalloc(sizeof(struct small_node),GFP_KERNEL);
		new->data=i;
		tiered_list_add(new,&my_list);
	}

	i=1;
	ktime_get_ts(&spclock[0]);
	list_for_each(p,&my_list.small_head){
		current_node=list_entry(p,struct small_node,head);
		if (i==500){
			printk("search data:(data,idx):(%d,%d)\n",current_node->data,current_node->idx);
			break;
		}
		current_node=list_entry(p,struct small_node,head);
		i=i+1;
		
	}
	ktime_get_ts(&spclock[1]);
	calclock3(spclock, &total_time);
	printk("Time1:%lld\n",total_time);
	total_time=0;
	/*
	list_for_each(p,&my_list.small_head){
		current_node=list_entry(p,struct small_node,head);
		printk("(data,idx):(%d,%d), (bighead : %p)\n",current_node->data,current_node->idx, current_node->parent);
		
	}
	
	int count_before_delete=0;
	list_for_each(p,&my_list.big_head){
		count_before_delete=count_before_delete+1;
		
	}
	printk("big head count before delete:%d\n",count_before_delete);
	
	printk("___________________________________________________________________");
	
	tiered_list_delete(&my_list);
	list_for_each(p,&my_list.small_head){
		current_node=list_entry(p,struct small_node,head);
		printk("(data,idx):(%d,%d)\n",current_node->data,current_node->idx);
		
	}
	int count_before_after=0;	// number of big_head
	list_for_each(p,&my_list.big_head){
		count_before_after=count_before_after+1;
		
	}
	printk("big head count before after:%d\n",count_before_after);


	printk("___________________________________________________________________");
	
	tiered_list_delete_tail(&my_list);
	list_for_each(p,&my_list.small_head){
		current_node=list_entry(p,struct small_node,head);
		printk("(data,idx):(%d,%d)\n",current_node->data,current_node->idx);
		
	}
	count_before_after=0;
	list_for_each(p,&my_list.big_head){
		count_before_after=count_before_after+1;
		
	}
	printk("big head count before after:%d\n",count_before_after);
	
	printk("___________________________________________________________________");
	*/

	struct small_node *res;
	ktime_get_ts(&spclock[0]);
	res = get(500, &my_list);
	ktime_get_ts(&spclock[1]);
	calclock3(spclock, &total_time);
	
	printk("get i_th node from the list : (data:%d, idx:%d)\n", res->data, res->idx);
	printk("Time2:%lld\n",total_time);
	/*
	res = get(9, &my_list);
	printk("get i_th node from the list : (data:%d, idx:%d)\n", res->data, res->idx);
	*/
}

int __init tiered_list_module_init(void){
	printk("tiered_list_list init\n");
	
	tiered_list_init(&my_list);
	test1();

	

	return 0;
}

void __exit tiered_list_cleanup(void){

	printk("tiered_list_list bye\n");
}

module_init(tiered_list_module_init);
module_exit(tiered_list_cleanup);

MODULE_LICENSE("GPL");
