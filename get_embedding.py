import numpy as np
import networkx as nx
import matplotlib.pyplot as plt
from scipy.sparse import csr_matrix
from scipy.sparse.linalg import eigsh
from sklearn.decomposition import PCA
from scipy.sparse.linalg import lobpcg
from sklearn.cluster import DBSCAN
from scipy.sparse import csc_matrix
from scipy.sparse.linalg import inv
from scipy.linalg import cholesky
from scipy.sparse.linalg import spilu
from scipy.sparse.linalg import LinearOperator
from sklearn.cluster import SpectralClustering
import umap

def read_graph(file_name):
    with open(file_name, 'r') as f:
        lines = f.readlines()
    n_edges, n_vertices, _ = map(int, lines[0].split())
    edges = []
    for line in lines[1:n_edges + 1]:
        # first two numbers are float and next two are integers
        _, _, v1, v2 = map(float, line.split())
        # convert to integers
        v1, v2 = int(v1), int(v2)
        edges.append((v1 - 1, v2 - 1))  # assuming 1-based index in file, convert to 0-based
    vertex_weights = np.array(list(map(float, lines[n_edges + 1:n_edges + n_vertices + 1])))
    return n_vertices, edges, vertex_weights

def construct_laplacian(n_vertices, edges, vertex_weights):
    row_indices = []
    col_indices = []
    data = []
    for v1, v2 in edges:
        row_indices.extend([v1, v2])
        col_indices.extend([v2, v1])
        data.extend([1, 1])
    
    # if row_indices exceed n_vertices, it means there is an error in the input file
    assert max(row_indices) < n_vertices
    assert max(col_indices) < n_vertices

    # Construct the adjacency matrix
    A = csr_matrix((data, (row_indices, col_indices)), shape=(n_vertices, n_vertices))
    # Construct the degree matrix
    D = csr_matrix((np.sum(A, axis=1).flatten(), (range(n_vertices), range(n_vertices))), shape=(n_vertices, n_vertices))
    # Laplacian Matrix
    L = D - A
    return L

def read_weighted_graph_and_create_laplacian(file_name):
    G = nx.Graph()
    with open(file_name, 'r') as f:
        lines = f.readlines()
    
    n_edges, n_vertices, _ = map(int, lines[0].split())
    
    # Read edges
    for line in lines[1:n_edges + 1]:
        _, _, v1, v2 = map(float, line.split())
        v1, v2 = int(v1), int(v2)
        G.add_edge(v1 - 1, v2 - 1)  # adjust index if needed
    
    # Read vertex weights
    vertex_weights = list(map(float, lines[n_edges + 1:n_edges + n_vertices + 1]))
    
    # Apply vertex weights
    for idx, weight in enumerate(vertex_weights):
        if idx in G:
            G.nodes[idx]['weight'] = weight
    
    # Create weighted Laplacian matrix
    L = nx.laplacian_matrix(G, weight='weight', nodelist=sorted(G.nodes())).toarray()
    
    if L.dtype == object:
        L = L.astype(np.float64)
        
    L = L.astype(np.float64)
    return L

def read_graph_and_create_laplacian(file_name):
    G = nx.Graph()
    with open(file_name, 'r') as f:
        lines = f.readlines()
    n_edges, n_vertices, _ = map(int, lines[0].split())
    #n_edges, n_vertices = map(int, lines[0].split())
    for line in lines[1:n_edges + 1]:
        _, _, v1, v2 = map(float, line.split())
        v1, v2 = int(v1), int(v2)
        #v1, v2 = map(int, line.split())
        G.add_edge(v1 - 1, v2 - 1)  # adjust index if needed

    vertex_weights = np.array(list(map(float, lines[n_edges + 1:n_edges + n_vertices + 1])))
    # Get the Laplacian matrix as a numpy array
    L = nx.laplacian_matrix(G)
    L = L.astype(np.float64)
    # visualize the laplacian matrix 
    plt.figure(figsize=(8, 6))
    plt.imshow(L, cmap='hot', interpolation='nearest')
    plt.colorbar()
    plt.title('Laplacian Matrix Visualization')
    plt.xlabel('Node Index')
    plt.ylabel('Node Index')
    plt.savefig('laplacian_matrix.png')
    return L

def get_clusters_from_embedding(file_name):
    L = read_weighted_graph_and_create_laplacian(file_name)
    num_eigenvectors = L.shape[0]
    eigenvalues, eigenvectors = eigsh(L, k=num_eigenvectors, which='SM', sigma=1e-10)
    # run dbscan to find clusters
    dbscan = DBSCAN(eps = 0.025, min_samples=5)
    # get the smallest two eigenvectors
    eigenvector_subset = eigenvectors[:, :2]
    clusters = dbscan.fit_predict(eigenvector_subset)
    # get the cluster labels
    # get the type of clusters
    clusters = clusters.astype(np.int32)
    return clusters

def main():
    """
    n_vertices, edges, vertex_weights = read_graph('block_level_netlist_4_14_4_200_1.hgr')
    print("Number of vertices:", n_vertices)
    print("Sample edges:", edges[:5])  # print first few to avoid flooding the output
    print("Sample vertex weights:", vertex_weights[:5])  # similarly, print a few weights

    L = construct_laplacian(n_vertices, edges, vertex_weights)
    print("Laplacian matrix:")
    print(L)
    # Calculate the smallest two non-zero eigenvalues and corresponding eigenvectors
    eigenvalues, eigenvectors = eigsh(L, k=2, which='SM', sigma=1e-5)  # shift to exclude zero eigenvalue
    print("Smallest two eigenvalues:", eigenvalues)
    print("Corresponding eigenvectors:", eigenvectors)
    """

    clusters = get_clusters_from_embedding('block_level_netlist_4_14_4_400_1.hgr')

    """
    #L = read_graph_and_create_laplacian('block_level_netlist_4_14_4_400_1.hgr')
    L = read_weighted_graph_and_create_laplacian('block_level_netlist_4_14_4_400_1.hgr')
    print("Number of nonzeros in L : ", np.count_nonzero(L))
    num_eigenvectors = L.shape[0]
    #eigenvalues, eigenvectors = eigsh(L, k=num_eigenvectors, which='SM', sigma=1e-10)  # shift to exclude zero eigenvalue

    L_csc = csc_matrix(L)
    ilu_preconditioner = spilu(L_csc)
    
    # Define a LinearOperator to use the preconditioner
    M = LinearOperator(matvec=ilu_preconditioner.solve, dtype=np.float64, shape=L_csc.shape)
    
    # Initial guess for the eigenvectors
    X = np.random.rand(L_csc.shape[0], num_eigenvectors)

    # Compute the smallest eigenvalues and corresponding eigenvectors
    eigenvalues, eigenvectors = lobpcg(L_csc, X, M=M, largest=False, tol=1e-10, maxiter=1000)

    # run dbscan to find clusters
    dbscan = DBSCAN(eps = 0.025, min_samples=5)
    # get the smallest two eigenvectors
    eigenvector_subset = eigenvectors[:, :2]
    clusters = dbscan.fit_predict(eigenvector_subset)

    # Plot the clustering results
    plt.figure(figsize=(8, 6))
    plt.scatter(eigenvector_subset[:, 0], eigenvector_subset[:, 1], c=clusters, cmap='viridis', alpha=0.6, edgecolors='w', linewidth=0.5)
    plt.title('DBSCAN Clustering of Graph Embedding')
    plt.xlabel('Eigenvector Component 1')
    plt.ylabel('Eigenvector Component 2')
    plt.grid(True)
    plt.savefig('graph_eigs_dbscan.png')
    print("Smallest", len(eigenvalues), " eigenvalues :", eigenvalues)

    # get the top 4 nodes in L with highest degrees
    degrees = np.diag(L)
    print("Degrees of all nodes: ", degrees)
    top_nodes = np.argsort(degrees)[-4:]
    print("Top 4 nodes with highest degrees: ", top_nodes)
    print("Degrees of top 4 nodes: ", degrees[top_nodes])

    # Plotting the reduced data
    plt.figure(figsize=(10, 8), dpi=300)
    plt.scatter(eigenvectors[:, 0], eigenvectors[:, 1], alpha=0.6, edgecolors='w', linewidth=0.5)

    for node in top_nodes:
        plt.scatter(eigenvectors[node, 0], eigenvectors[node, 1], color='green', edgecolors='k', linewidth=2, s=100, label=f'Node {node}')

    plt.title('2D Projection of Graph Embedding Using Eigenvectors')
    plt.xlabel('Eigenvector Component 1')
    plt.ylabel('Eigenvector Component 2')
    plt.grid(True)
    # save the plot 
    plt.savefig('graph_eigs_embedding.png')

    pca = PCA(n_components=2)
    reduced_data = pca.fit_transform(eigenvectors)

    # Plotting the reduced data
    plt.figure(figsize=(10, 8), dpi=300)
    plt.scatter(reduced_data[:, 0], reduced_data[:, 1], alpha=0.6, edgecolors='w', linewidth=0.5)
    # Highlight specific nodes
    #highlighted_nodes = [1, 49, 97, 145]
    #for node in highlighted_nodes:
    #    plt.scatter(reduced_data[node, 0], reduced_data[node, 1], color='red', edgecolors='k', linewidth=2, s=100, label=f'Node {node}')

    for node in top_nodes:
        plt.scatter(reduced_data[node, 0], reduced_data[node, 1], color='green', edgecolors='k', linewidth=2, s=100, label=f'Node {node}')

    # Add legend for highlighted nodes to avoid duplicate labels
    handles, labels = plt.gca().get_legend_handles_labels()
    by_label = dict(zip(labels, handles))
    plt.legend(by_label.values(), by_label.keys())

    plt.title('2D Projection of Graph Embedding Using PCA')
    plt.xlabel('Principal Component 1')
    plt.ylabel('Principal Component 2')
    plt.grid(True)
    plt.savefig('graph_eigs_pca_embedding.png')

    reducer = umap.UMAP(random_state=42, n_neighbors=50, min_dist=0.01, n_components=2)
    umap_embedding = reducer.fit_transform(eigenvectors)

    # Assuming you know the indices of your high-degree nodes:
    high_degree_nodes_indices = [0, 48, 96, 144]

    # Plotting the UMAP embedding
    plt.figure(figsize=(12, 10), dpi=300)
    plt.scatter(umap_embedding[:, 0], umap_embedding[:, 1], alpha=0.6, edgecolors='w', linewidth=0.5)

    # Highlight high-degree nodes
    for node in high_degree_nodes_indices:
        plt.scatter(umap_embedding[node, 0], umap_embedding[node, 1], color='red', edgecolors='k', linewidth=2, s=100, label=f'Node {node}')

    # Adding a legend and titles
    handles, labels = plt.gca().get_legend_handles_labels()
    by_label = dict(zip(labels, handles))
    plt.legend(by_label.values(), by_label.keys())
    plt.title('2D Projection of Graph Embedding Using UMAP')
    plt.xlabel('UMAP Component 1')
    plt.ylabel('UMAP Component 2')
    plt.grid(True)
    plt.savefig('graph_eigs_umap_embedding.png')
    """
    
if __name__ == '__main__':
    main()