//
// tree.h -- binary search tree
//           
// If a duplicate item is going to be added to the tree,
// the counter for the already existent item is incremented
//
// Binary Search Tree ADT - Interface declarations
//
// Based on listing 17.10 ( 'tree.h' - C Primer Plus - Prata - 5ed )
//

#ifndef _TREE_H_
#define _TREE_H_

#define     FALSE       0
#define     TRUE        1

#define     VALSTR      32

typedef struct item
{
    char nmeaVal[ VALSTR ]; // Raw nmea value (extra signed lat, lon)
    int intVal;             // Signed int full precision [ms] or [dm]
    double dblVal;          // Signed double end units [deg] or [m]
    int ct;                 // Count of pts with this value
    double wtVal;           // Weighted value [deg] or [m]
} Item;

typedef struct node
{
    Item item;
    struct node* left;      // pointer to right branch
    struct node* right;     // pointer to left branch
} Node;

typedef struct tree
{
    Node* root;             // pointer to root of tree
    int ctTotNodes;         // number of nodes in tree
    int ctTotMeas;          // Total measurements considered
    double wtTotVal;        // Total weighted result [deg] or [m]
} Tree;

/* function prototypes */

/* operation:      initialize a tree to empty          */
/* preconditions:  ptree points to a tree              */
/* postconditions: the tree is initialized to empty    */
void InitializeTree( Tree* ptree );

/* operation:      determine if tree is empty          */
/* preconditions:  ptree points to a tree              */
/* postconditions: function returns true if tree is    */
/*                 empty and returns false otherwise   */
int TreeIsEmpty( const Tree* ptree );

/* operation:      determine if tree is full           */
/* preconditions:  ptree points to a tree              */
/* postconditions: function returns true if tree is    */
/*                 full and returns false otherwise    */
int TreeIsFull( const Tree* ptree );

/* operation:      determine number of items in tree   */
/* preconditions:  ptree points to a tree              */
/* postconditions: function returns number of items in */
/*                 tree                                */
int TreeItemCount( const Tree* ptree );

/* operation:      add an item to a tree               */
/* preconditions:  pi is address of item to be added   */
/*                 ptree points to an initialized tree */
/* postconditions: if possible, function adds item to  */
/*                 tree and returns true; otherwise,   */
/*                 the function returns false          */
int AddItem( const Item* pi, Tree* ptree );

/* operation:      find an item in a tree              */
/* preconditions:  pi points to an item                */
/*                 ptree points to an initialized tree */
/* postconditions: function returns true if item is in */
/*                 tree and returns false otherwise    */
int InTree( const Item* pi, const Tree* ptree );

/* operation:      delete an item from a tree          */
/* preconditions:  pi is address of item to be deleted */
/*                 ptree points to an initialized tree */
/* postconditions: if possible, function deletes item  */
/*                 from tree and returns true;         */
/*                 otherwise, the function returns false*/
int DeleteItem( const Item* pi, Tree* ptree );

/* operation:      apply a function to each item in    */
/*                 the tree                            */
/* preconditions:  ptree points to a tree              */
/*                 pfun points to a function that takes*/
/*                 an Item argument and has no return  */
/*                 value                               */
/* postcondition:  the function pointed to by pfun is  */
/*                 executed once for each item in tree */
void Traverse( Tree* ptree, void ( *pfun )( Item* itemPt, int val ) );

/* operation:      get total weighted value from tree  */
/* preconditions:  ptree points to a tree              */
/* postcondition:  total weighted value is retrieved   */
double TraverseWtVal( Tree* ptree );

/* operation:      delete everything from a tree       */
/* preconditions:  ptree points to an initialized tree */
/* postconditions: tree is empty                       */
void DeleteAll( Tree* ptree );

#endif
