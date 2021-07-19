import PluginImpl from './plugin';

const instance = new PluginImpl;

export function getLabel(): ArrayBuffer {
    return String.UTF8.encode(instance.getLabel(), true);
}

export function getMaker(): ArrayBuffer {
    return String.UTF8.encode(instance.getMaker(), true);
}

export function getLicense(): ArrayBuffer {
    return String.UTF8.encode(instance.getLicense(), true);
}