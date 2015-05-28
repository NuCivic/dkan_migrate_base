<?php

/**
 * This file provides a class that enables superclasses to call the import of
 * resources from CKAN.
 */

/**
 * Class MigrateCkanResources
 */
abstract class MigrateCkanResourcesBase extends MigrateCkanDatasetBase {

  private $machine_name = '';

  public function __construct($arguments) {
    parent::__construct($arguments);

    $this->machine_name = $arguments['machine_name'];
  }

  /**
   * Overload the prepareRow function to import the resources. Then run the
   * standard import.
   *
   * @param object $row
   */
  public function prepareRow($row) {
    // Migrate the resources for this dataset.
    $this->migrateResources($row->resources);

    parent::prepareRow($row);
  }

  /**
   * Migrate the specified resources.
   *
   * @param array $resources An array of resources ids.
   */
  protected function migrateResources($resources) {

    // Return if there are no resources.
    if (!is_array($resources)) {
      return;
    }

    $machine_name = $this->machine_name . '_resources';
    $filename     = "public://$machine_name";

    drupal_set_message(t('Migrating ' . sizeof($resources) . ' resource(s) [ machine name :: ' . $machine_name . ' ][ filename :: @filename]...', array('@filename' => $filename)));

    file_unmanaged_save_data(json_encode($resources), $filename, FILE_EXISTS_REPLACE);

    $current_migration = self::$currentMigration;
    // Register the migration, then get its instance and start it.
    MigrationBase::registerMigration('MigrateCkanResources', $machine_name, array(
      'migrate_list' => $filename,
      'migrate_item' => $filename,
    ));

    echo("Getting migration instance [ machine name :: $machine_name ]\n");
    $migration = Migration::getInstance($machine_name);

    echo('Processing import [ migration :: ' . (isset($migration) ? 'true' : 'false') . ' ]' . "\n");
    $result = $migration->processImport();

    echo("De-registering the migration [ machine name :: $machine_name ]\n");
    // Remove the migration.
     MigrationBase::deregisterMigration($machine_name);

    self::$currentMigration = $current_migration;

    file_unmanaged_delete($filename);

    echo("Migration completed [ result :: $result ][ machine name :: $machine_name ]\n");

  }

}

class DkanMigrateList extends MigrateList {

  private $items;

  public function __construct($items) {
    parent::__construct();

    $this->items = $items;
  }

  /**
   * Implementors are expected to return a string representing where the listing
   * is obtained from (a URL, file directory, etc.)
   *
   * @return string
   */
  public function __toString() {
    // TODO: Implement __toString() method.
    return 'TODO: ...';
  }

  /**
   * Implementors are expected to return an array of unique IDs, suitable for
   * passing to the MigrateItem class to retrieve the data for a single item.
   *
   * @return Mixed, iterator or array
   */
  public function getIdList() {

    return array_keys($this->items);
  }

  /**
   * Implementors are expected to return a count of IDs available to be migrated.
   *
   * @return int
   */
  public function computeCount() {

    return (is_array($this->items) ? sizeof($this->items) : -1);
  }

  /**
   * Implementors are expected to return a count of IDs available to be migrated.
   *
   * @return int
   */
  public function count() {

    return $this->computeCount();
  }
}

class DkanMigrateItem extends MigrateItem {

  private $items;

  public function __construct($items = array()) {
    parent::__construct();

    $this->$items = $items;
  }

  /**
   * Implementors are expected to return an object representing a source item.
   *
   * @param mixed $id
   *
   * @return stdClass
   */
  public function getItem($id) {

    return $this->items[$id];
  }
}

/**
 * A dynamic migration that is reused for each source CSV file.
 */
class MigrateCkanResources extends MigrateCkanBase {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);

    // Allow superclass to override the list of fields.
    // TODO: candidate to be moved to the superclass.
    $fields = $this->getCkanResourceFields();

    $this->highwaterField = array(
      'name' => 'revision_timestamp',
    );

    // TODO: candidate to be moved to the superclass.
    $list_url = isset($arguments['list_url']) ? $arguments['list_url'] : 'resource_list';
    $item_url = isset($arguments['item_url']) ? $arguments['item_url'] : 'resource_show?id=:id';

    // Get the shared key for multi-threaded imports, or use the machine name.
    $this->shared_key = isset($arguments['shared_key']) ? $arguments['shared_key'] : $this->machineName;

    // TODO: candidate to be moved to the superclass.
    // The following are provided by the multi-threaded environment.
    $this->limit  = isset($arguments['limit']) ? $arguments['limit'] : '';
    $this->offset = isset($arguments['offset']) ? $arguments['offset'] : '';

//    if ($list_url == 'resource_list') {
//      $file_name = 'public://ckan-migrate-resource_list_' . get_class($this);
//      if (!file_exists($file_name)) {
//        drupal_set_message('resource_show created');
//        // CKAN doesn't have a resource_list endpoint. This function fakes it.
//        dkan_migrate_base_create_resource_list($this->endpoint, $file_name, TRUE);
//        variable_set('dkan_migrate_base_ckan-migrate-resource_list', date('r', time()));
//      }
//      else {
//        $date = variable_get('dkan_resource_list_' . get_class($this), t('unknown'));
//        drupal_set_message(t('ckan-migrate-resource_list file downloaded locally. Last updated %date.', array('%date' => $date)));
//      }
//      $list_url = file_create_url($file_name);
//    }

//    $item_url = $this->endpoint . $item_url;

    // TODO: candidate to be moved to the superclass.

    $filename = drupal_realpath($arguments['migrate_list']);
    if (empty($filename)) {
      drupal_set_message('This migration needs a filename', 'warn');
      return;
    }
    $resources = json_decode(file_get_contents($filename));

//    $this->migrate_list = new DkanMigrateList($resources);
//    $this->migrate_item = new DkanMigrateItem($resources);

//    echo("Resources loaded [ count :: " . sizeof($resources) . " ][ filename :: $filename ][ compute count :: " . $this->migrate_list->computeCount() . " ]\n");

    $this->source = new MigrateSourceList(new ExampleListJSON($filename),
      new ExampleItemJSON($filename, array()), $fields);

//    $this->source = new MigrateSourceList($this->migrate_list, $this->migrate_item, $fields);

    $this->map = new MigrateSQLMap(
      $this->shared_key,
      array(
        'uuid' => array(
          'type'        => 'varchar',
          'length'      => 255,
          'not null'    => TRUE,
          'description' => 'id',
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $this->destination = new MigrateDestinationNode('resource', array('text_format' => 'html'));
    $this->addFieldMapping('id', 'uuid');
    $this->addFieldMapping('title', 'name');
    $this->addFieldMapping('created', 'created');
    $this->addFieldMapping('body', 'description');
    $this->addFieldMapping('field_format', 'format');
    $this->addFieldMapping('field_upload', 'file');
    $this->addFieldMapping('field_link_remote_file', 'file_remote_link');
    $this->addFieldMapping('field_link_api', 'link');
    $this->addFieldMapping('og_group_ref', 'group_ids');
    $this->addFieldMapping('changed', 'last_modified');
  }

  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {

    echo("Preparing row");
    // TODO:
    // + Find way to get user name for creator of resource
    // + Improve preview for files stuck as links.
    $row->created       = $this->StringToTime($row->created);
    $row->created       = $this->StringToTime($row->last_modified);
    $row->group_ids     = array($row->resource_group_id);
    $row->last_modified = $this->StringToTime($row->last_modified);

    $row->name = $row->name ? $row->name : $row->description;

    migrate_instrument_start("taxonomy_vocabulary_machine_name_load");

    // Tax terms in Drupal are case sensitive. It is better to have a single
    // 'html' term instead of 'html' and 'HTML'.
    // TODO: move to hook_node_update in dkan_dataset.
    $format      = taxonomy_vocabulary_machine_name_load('format');
    $row->format = strtolower($row->format);
    $this->createTax($row->format, 'format', $format->vid);

    migrate_instrument_start("taxonomy_vocabulary_machine_name_load");

    // Decide which of DKAN's three fields is best for resource file.
    if ($row->url_type == 'upload' || $row->resource_type == 'file.upload' || $row->resource_type == 'file') {
      $name = explode('/', $row->url);
      $name = $name[count($name) - 1];
      $uri  = 'public://' . $name;

      migrate_instrument_start("downloadExternalFile [ url :: $row->url ]");

      $file = $this->downloadExternalFile($row->url, $uri);

      migrate_instrument_stop("downloadExternalFile [ url :: $row->url ]");

      $row->file = $file['fid'];
    }
    else {
      // CKAN API doesn't make it clear if file is link to API or just file.
      $field      = field_info_instance('node', 'field_link_remote_file', 'resource');
      $extensions = explode(' ', $field['settings']['file_extensions']);
      if (in_array($row->format, $extensions) && $row->format != 'html') {
        $row->file_remote_link = trim($row->url);
      }
      else {
        $row->link = $row->url;
      }
    }
  }

  /**
   * Implements prepare.
   */
  public function prepare($node, stdClass $row) {
    // UUID doesn't show up as a field so had to do this.
    $node->uuid  = $row->id;
    $row->format = isset($row->format) && $row->format ? $row->format : 'data';
  }

  public function getCkanResourceFields() {
    return array(
      "name"           => "Name",
      "id"             => "UUID",
      "description"    => "Description",
      "format"         => "Format",
      "created"        => "Created Date",
      "last_modified"  => "Modified Date",
      "url"            => "URL",
      "revision_id"    => "Revision ID",
      // Not sure if this is just Mimetype.
      "mimetype_inner" => "Mimetype Inner",
      // This doesn't actually exist but we are adding it later in prepareRow.
      "uid"            => "User ID",
      "file"           => "file",
    );
  }
}

class ExampleListJSON extends MigrateListJSON {
  /**
   * The default implementation assumes the IDs are top-level array elements,
   * but the array elements are the data items - we need to look inside them
   * for the IDs.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    foreach ($data as $item) {
      $ids[] = $item['id'];
    }
    return $ids;
  }
}

class ExampleItemJSON extends MigrateItemJSON {
  protected $data = array();
  /**
   * Two problems with the default getItem() - it reads and parses the JSON on
   * each call, which we don't want to do for multiple items in the file; and,
   * we need to retrieve a given item from the file according to its 'id' element.
   */
  public function getItem($id) {
    // We cache the parsed JSON at $this->data.
    if (empty($this->data)) {
      $data = $this->loadJSONUrl($this->itemUrl);
      if ($data) {
        // Let's index the array by the ID for easy retrieval.
        foreach ($data as $item) {
          $this->data[$item->id] = $item;
        }
      }
      else {
        // Error-handling here....
      }
    }
    // Return the requested item
    if (isset($this->data[$id])) {
      return $this->data[$id];
    }
    else {
      return NULL;
    }
  }
}