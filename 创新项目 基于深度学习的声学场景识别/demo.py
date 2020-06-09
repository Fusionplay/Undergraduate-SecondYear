from .code.cnn import SceneNetwork
from .code.loader import FeaturesLoader, DemoLoader


def demo(evaluation_dir):
    train_dir      = r'F:\projectdata\train'
    validation_dir = r'F:\projectdata\valid'

    # labels = ['bus', 'cafe_restaurant', 'beach', 'city_center', 'forest_path', 'car', 'grocery_store', 'home',
    #          'library', 'metro_station', 'office', 'park', 'residential_area', 'train', 'tram']
    labels = ['airport-barcelona', 'bus-london', 'metro-barcelona', 'metro_station-barcelona', 'park-barcelona',
              'public_square-barcelona',
              'shopping_mall-barcelona', 'street_pedestrian-barcelona', 'street_traffic-barcelona', 'tram-barcelona']
    # labels=['war','veh','str','rai','pla','for','bea','bar','the','tra']
    batch_size = 32

    # sampling_rate = 48000.0
    sampling_rate = 48000.0
    hop_size = 512.0
    time_duration = 10.0
    t_dim = (int(time_duration * sampling_rate / hop_size) + 1) * 2  # *2 for both *1 for MFCCs

    n_features = 20

    input_shape = (n_features, t_dim, 1)

    evaluation_loader = DemoLoader(evaluation_dir, labels, n_features=n_features)
    print('123')
    scene_network = SceneNetwork()
    print('qwe')
    scene_network.load_model(r'F:\projectdata\models\model.h5')
    print('456')

    x_eval, y_eval = evaluation_loader.get_data()
    print('789')
    pred = scene_network.Demo_evaluate(x_eval, y_eval)
    print(type(pred[0]))
    print('pred=', labels[pred[0]])
    return labels[pred[0]]
