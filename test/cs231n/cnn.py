import h5py
import numpy as np
import tensorflow as tf
from tensorflow.keras import layers, models
import matplotlib.pyplot as plt

def load_data(file_path, group_name):
    with h5py.File(file_path, 'r') as f:
        group = f[group_name]
        beamE = group['beamE'][:]
        hist2d_data = group['hist2d_data'][:]
        
        esum = np.sum(hist2d_data, axis=(1, 2))
        
    return beamE, hist2d_data, esum

def concatenate_data(data_list):
    concatenated_beamE = np.concatenate([data[0] for data in data_list], axis=0)
    concatenated_hist2d_data = np.concatenate([data[1] for data in data_list], axis=0)
    concatenated_esum = np.concatenate([data[2] for data in data_list], axis=0)
    return concatenated_beamE, concatenated_hist2d_data, concatenated_esum

def split_data(beamE, hist2d_data, esum, labels_particle, split_ratio=(0.7, 0.2, 0.1)):
    total_samples = beamE.shape[0]
    indices = np.arange(total_samples)
    np.random.shuffle(indices)

    train_end = int(split_ratio[0] * total_samples)
    val_end = train_end + int(split_ratio[1] * total_samples)

    train_indices = indices[:train_end]
    val_indices = indices[train_end:val_end]
    test_indices = indices[val_end:]

    train_beamE, val_beamE, test_beamE = beamE[train_indices], beamE[val_indices], beamE[test_indices]
    train_hist2d_data, val_hist2d_data, test_hist2d_data = hist2d_data[train_indices], hist2d_data[val_indices], hist2d_data[test_indices]
    train_esum, val_esum, test_esum = esum[train_indices], esum[val_indices], esum[test_indices]
    train_labels_particle, val_labels_particle, test_labels_particle = labels_particle[train_indices], labels_particle[val_indices], labels_particle[test_indices]

    return (train_beamE, train_hist2d_data, train_esum, train_labels_particle), \
           (val_beamE, val_hist2d_data, val_esum, val_labels_particle), \
           (test_beamE, test_hist2d_data, test_esum, test_labels_particle)

def build_model(input_shape):
    input_img = layers.Input(shape=input_shape, name='input_img')
    input_esum = layers.Input(shape=(1,), name='input_esum')

    x = layers.Conv2D(64, (5, 5), activation='relu')(input_img)
    x = layers.Conv2D(32, (3, 3), activation='relu')(x)
    x = layers.MaxPooling2D((2, 2))(x)
    x = layers.Conv2D(32, (3, 3), activation='relu')(x)
    x = layers.Conv2D(32, (3, 3), activation='relu')(x)
    x = layers.BatchNormalization()(x)
    x = layers.MaxPooling2D((2, 2))(x)
    x = layers.Conv2D(32, (3, 3), activation='relu')(x)
    x = layers.Conv2D(32, (3, 3), activation='relu')(x)
    x = layers.Conv2D(6, (3, 3), activation='relu')(x)
    x = layers.MaxPooling2D((2, 2))(x)

    x = layers.Flatten()(x)

    x = layers.Dense(512, activation='relu')(x)
    x = layers.Dropout(0.2)(x)  
    x = layers.Dense(128, activation='relu')(x)
    x = layers.Dropout(0.2)(x) 
    x = layers.Dense(32, activation='relu')(x)

    x = layers.Concatenate()([x, input_esum])

    output_particle = layers.Dense(4, activation='softmax', name='output_particle')(x)

    output_energy = layers.Dense(1, name='output_energy')(x)

    model = models.Model(inputs=[input_img, input_esum], outputs=[output_particle, output_energy])
    return model

particle_types = ['e+', 'e-', 'pi+', 'pi-']

data_list = []
for particle in particle_types:
    file_path = f'{particle}_E1-100_2000.h5'
    group_name = f'{particle}_E1-100_2000'
    data_list.append(load_data(file_path, group_name))

beamE, hist2d_data, esum = concatenate_data(data_list)

particles_type = np.concatenate([np.full(2000, i) for i in range(4)], axis=0)
labels_particle = tf.keras.utils.to_categorical(particles_type, num_classes=4)

hist2d_data = hist2d_data.reshape((-1, 57, 49, 1))

(train_beamE, train_hist2d_data, train_esum, train_labels_particle), \
(val_beamE, val_hist2d_data, val_esum, val_labels_particle), \
(test_beamE, test_hist2d_data, test_esum, test_labels_particle) = split_data(beamE, hist2d_data, esum, labels_particle)

input_shape = (57, 49, 1)
model = build_model(input_shape)
model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
              loss={'output_particle': 'categorical_crossentropy', 'output_energy': 'mean_squared_logarithmic_error'},
              metrics={'output_particle': 'accuracy', 'output_energy': 'mae'})

early_stopping = tf.keras.callbacks.EarlyStopping(monitor='val_loss', patience=6)
checkpoint = tf.keras.callbacks.ModelCheckpoint('best_model.keras', monitor='val_loss', save_best_only=True)

history = model.fit([train_hist2d_data, train_esum],
                    [train_labels_particle, train_beamE.reshape((-1, 1))],
                    validation_data=([val_hist2d_data, val_esum], [val_labels_particle, val_beamE.reshape((-1, 1))]),
                    epochs=25,
                    batch_size=128,
                    callbacks=[early_stopping, checkpoint])

def plot_loss(history):
    plt.figure(figsize=(12, 6))
    plt.plot(history.history['loss'], label='Training Loss')
    plt.plot(history.history['val_loss'], label='Validation Loss')
    plt.xlabel('Epochs')
    plt.ylabel('Loss')
    plt.legend()
    plt.title('Training and Validation Loss')
    plt.savefig('loss_vs_epoch.png')

plot_loss(history)

model = tf.keras.models.load_model('best_model.keras')

loss, acc_particle, mae_energy = model.evaluate([test_hist2d_data, test_esum], [test_labels_particle, test_beamE.reshape((-1, 1))])
print(f'Test Loss: {loss}, Test Particle Accuracy: {acc_particle}, Test Energy MAE: {mae_energy}')

predictions_particle, predictions_energy = model.predict([test_hist2d_data, test_esum])
predictions_particle_classes = np.argmax(predictions_particle, axis=1)

particle_accuracy = np.mean(predictions_particle_classes == np.argmax(test_labels_particle, axis=1))
print(f'Test Particle Type Accuracy: {particle_accuracy}')

plt.figure(figsize=(12, 6))
plt.hist(np.argmax(test_labels_particle, axis=1), alpha=0.5, label='True Particle Types')
plt.hist(predictions_particle_classes, alpha=0.5, label='Predicted Particle Types')
plt.xlabel('Particle Types')
plt.ylabel('Frequency')
plt.legend()
plt.title('True vs Predicted Particle Types')
plt.savefig('particle_types_histogram.png')

plt.figure(figsize=(12, 6))
plt.scatter(test_beamE.reshape((-1)), predictions_energy, alpha=0.5, label='Predicted vs True Energy')
plt.xlabel('True Energy')
plt.ylabel('Predicted Energy')
plt.legend()
plt.title('True vs Predicted Energy')
plt.savefig('energy_scatter.png')
