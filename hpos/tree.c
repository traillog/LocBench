//
// tree.c -- tree support functions
//
// Binary Search Tree ADT - Interface implementation
//
// Based on listing 17.11 ( 'tree.c' - C Primer Plus - Prata - 5ed )
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tree.h"

/* local data type */
typedef struct pair
{
    Node* parent;
    Node* child;
} Pair;

/* protototypes for local functions */
static Node* MakeNode( const Item* pi );
static int ToLeft( const Item* i1, const Item* i2 );
static int ToRight( const Item* i1, const Item* i2 );
static void AddNode( Node* new_nodePt, Node* root );
static void InOrder( const Node* root, void ( *pfun )( Item item ) );
static Pair SeekItem( const Item* pi, const Tree* ptree );
static void DeleteNode( Node** ptr );
static void DeleteAllNodes( Node* ptr );

/* function definitions */
void InitializeTree( Tree* ptree )
{
    ptree->root = NULL;
    ptree->size = 0;
}

/* returns true if tree is empty */
int TreeIsEmpty( const Tree* ptree )
{
    if ( ptree->root == NULL )
        return TRUE;
    else
        return FALSE;
}

/* returns true if tree is full */
int TreeIsFull( const Tree* ptree )
{
    Node* pt;
    int full;

    // Create test node
    pt = ( Node* )malloc( sizeof( Node ) );

    // Validate allocation
    if ( pt == NULL )
        full = TRUE;
    else
        full = FALSE;

    // Destroy test node
    free( pt );

    return full;
}

int TreeItemCount( const Tree* ptree )
{
    return ptree->size;
}

int AddItem( const Item* pi, Tree* ptree )
{
    Node* new_nodePt;
    Pair serachRes = { 0 };

    // Check whether the tree has room
    // for a new node
    if  ( TreeIsFull( ptree ) )
    {
        fprintf( stderr, "Tree is full\n" );
        return FALSE;               /* early return           */
    }

    // Check whether item to add is already in the tree
    // If already in the tree --> increment count value
    serachRes = SeekItem( pi, ptree );
    if ( serachRes.child != NULL )
    {
        ++serachRes.child->item.count;
        return TRUE;
    }

    // Create a new node
    new_nodePt = MakeNode( pi );    /* points to new node     */
    if ( new_nodePt == NULL )
    {
        fprintf( stderr, "Couldn't create node\n" );
        return FALSE;               /* early return           */
    }

    /* succeeded in creating a new node */
    ptree->size++;

    if ( ptree->root == NULL )      /* case 1: tree is empty  */
        ptree->root = new_nodePt;   /* new node is tree root  */
    else                            /* case 2: not empty      */
        AddNode( new_nodePt, ptree->root );   /* add node to tree  */

    return TRUE;                    /* successful return      */
}

int InTree( const Item* pi, const Tree* ptree )
{
    return ( SeekItem( pi, ptree ).child == NULL ) ? FALSE : TRUE;
}

int DeleteItem( const Item* pi, Tree* ptree )
{
    Pair look;

    // Find node containing item to be deleted
    // address of node to be deleted goes in look.child
    // address of parent of node to be deleted goes in look.parent
    //
    // Pass to DeleteNode() the address of the pointer
    // containing the address of the node to be deleted
    look = SeekItem( pi, ptree );

    // If item not found --> nothing to delete --> return false
    if ( look.child == NULL )
        return FALSE;                   

    if ( look.parent == NULL )          /* delete root item       */
        DeleteNode( &ptree->root );
    else if ( look.parent->left == look.child ) // target node is
                                                // left child of its parent
        DeleteNode( &look.parent->left );
    else                                        // target node is
                                                // right child of its parent
        DeleteNode( &look.parent->right );

    // Decrement size of the tree
    ptree->size--;

    return TRUE;
}

void Traverse ( const Tree* ptree, void ( *pfun )( Item item ) )
{
    if ( ptree != NULL )
        InOrder( ptree->root, pfun );
}

// Delete the whole tree
void DeleteAll( Tree* ptree )
{
    // Delete all nodes if any
    if ( ptree != NULL )
        DeleteAllNodes( ptree->root );

    // Reset pointer to root
    ptree->root = NULL;

    // Reset node counter
    ptree->size = 0;
}


/* local functions */
static void InOrder( const Node* root, void ( *pfun )( Item item ) )
{
    if ( root != NULL )
    {
        // Process left subtree
        InOrder( root->left, pfun );

        // Process item in node
        ( *pfun )( root->item );

        // Process right subtree
        InOrder( root->right, pfun );
    }
}

static void DeleteAllNodes( Node* root )
{
    Node *pright;

    if ( root != NULL )
    {
        // Save address of right child
        pright = root->right;

        // Delete left subtree
        DeleteAllNodes( root->left );

        // Delete current node
        free( root );

        // Delete right subtree
        DeleteAllNodes( pright );
    }
}

// Called by AddItem()
// Determines where the new node goes and
// Adds the new node to the tree
//
// new_nodePt : address of the new node
// root       : address of the root of a tree/subtree
static void AddNode( Node* new_nodePt, Node* root )
{
    if ( ToLeft( &new_nodePt->item, &root->item ) )
    {
        // New node goes to the left of root
        if ( root->left == NULL )       /* empty left-subtree       */
            // Leaf found --> extend the tree here (!!!!!!!!!!!!!)
            root->left = new_nodePt;    /* so add node here         */
        else
            // Keep looking recursively into the left-subtree
            AddNode( new_nodePt, root->left );    /* else process subtree */
    }
    else if ( ToRight( &new_nodePt->item, &root->item ) )
    {
        // New node goes to the right of root
        if ( root->right == NULL )      /* empty right-subtree      */
            // Leaf found --> extend the tree here (!!!!!!!!!!!!!)
            root->right = new_nodePt;   /* so add node here         */
        else
            // Keep looking recursively into the right-subtree
            AddNode( new_nodePt, root->right );
    }
    else
    {
        // No place found for the new node
        fprintf( stderr, "location error in AddNode()\n" );
        exit( 1 );
    }
}

// Called by AddNode()
// Determines whether item of new node
// must precede the item of root
//
// i1 : address of item in new node
// i2 : address of item in current root node
static int ToLeft( const Item* i1, const Item* i2 )
{
    int comp1;

    if ( ( comp1 = strcmp( i1->val, i2->val ) ) < 0 )
        return TRUE;
    else 
        return FALSE;
}

// Called by AddNode()
// Determines whether item of root
// must precede the item of new node
//
// i1 : address of item in new node
// i2 : address of item in current root node
static int ToRight( const Item* i1, const Item* i2 )
{
    int comp1;

    if ( ( comp1 = strcmp( i1->val, i2->val ) ) > 0 )
        return TRUE;
    else 
        return FALSE;
}

// Called by AddItem()
// Creates a new node and
// initializes its contents
static Node* MakeNode( const Item* pi )
{
    Node* new_nodePt;

    // Create new node (allocate memory for it)
    new_nodePt = ( Node* )calloc( 1, sizeof( Node ) );

    if ( new_nodePt != NULL )
    {
        // Set up node content (Copy structures)
        new_nodePt->item = *pi;       

        // New node has no children (for the moment)
        new_nodePt->left = NULL;
        new_nodePt->right = NULL;
    }

    return new_nodePt;
}

// Called by AddItem(), InTree() and DeleteItem()
//
// Returns a structure containing
//      a pointer to the searched node and  ( .child  )
//      a pointer to its parent             ( .parent )
static Pair SeekItem( const Item* pi, const Tree* ptree )
{
    Pair look;

    // Initialize output structure 'look'
    look.parent = NULL;
    look.child = ptree->root;

    // Empty tree ?
    if ( look.child == NULL )
        return look;                        /* early return   */

    while ( look.child != NULL )
    {
        if ( ToLeft( pi, &( look.child->item ) ) )
        {
            look.parent = look.child;
            look.child = look.child->left;
        }
        else if ( ToRight( pi, &( look.child->item ) ) )
        {
            look.parent = look.child;
            look.child = look.child->right;
        }
        else       /* must be same if not to left or right    */
            break; /* look.child is address of node with item */
    }

    return look;                       /* successful return   */
}

static void DeleteNode( Node** ptr )
    /* ptr is address of parent member pointing to target node  */
    // ptr is address of pointer containing address of target node
{
    Node* temp;

    if ( ( *ptr )->left == NULL )       // target node has no left child
    {
        // Node to be deleted has no left-child
        // then, redirect parent to right-child
        temp = *ptr;          // store address of target node
        *ptr = (*ptr)->right; // redirect parent to right-child of deleted node
        free( temp );         // delete node
    }
    else if ( (*ptr)->right == NULL)    // target node has no right child
    {
        // Node to be deleted has no right-child
        // then, redirect parent to left-child
        temp = *ptr;            // store address of target node
        *ptr = (*ptr)->left;    // redirect parent to left-child of deleted node
        free( temp );           // delete node
    }
    else    /* target node has two children */
    {
        /* find where to reattach right subtree */
        for ( temp = ( *ptr )->left; temp->right != NULL; temp = temp->right )
            continue;

        // reattach right-subtree of target node
        // in the founded place
        temp->right = ( *ptr )->right;

        temp = *ptr;            // save address of target node
        *ptr = (*ptr)->left;     // redirect parent to left-child of target node
        free( temp );           // delete node
    }
}

