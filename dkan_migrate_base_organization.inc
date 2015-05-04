<?php

/**
 * @file
 * Migration Class for Groups
 */

/**
 * Migrate CKAN group
 */
class MigrateCkanOrganizationBase extends MigrateCkanBase {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $fields = $this->getCkanOrganizationFields();
    $list_url = $this->endpoint . 'organization_list';
    $item_url = $this->endpoint . 'organization_show?id=:id';
    $this->page = isset($arguments['page']) ? $arguments['page'] : '';
    $this->offset = isset($arguments['offset']) ? $arguments['offset'] : '';

    $this->source = new MigrateSourceList(new CKANListJSON(
      $list_url,
      array('page' => $this->page,
            'offset' => $this->offset,
      )
    ),
    new CKANItemJSON($item_url, $fields), $fields);

    $this->map = new MigrateSQLMap(
            $this->machineName,
            array(
              'uuid' => array(
                'type' => 'varchar',
                'length' => 255,
                'not null' => TRUE,
                'description' => 'id',
              ),
            ),
            MigrateDestinationNode::getKeySchema()
        );

    $this->destination = new MigrateDestinationNode('organization');
    $this->addFieldMapping('id', 'uuid');
    $this->addFieldMapping('uuid', 'id');
    $this->addFieldMapping('title', 'title');
    $this->addFieldMapping('body', 'description');
    $this->addFieldMapping('created', 'created');
    $this->addFieldMapping('changed', 'revision_timestamp');
    $this->addFieldMapping('path', 'name');
    $this->addFieldMapping('field_image', 'image_url');
  }

  /**
   * Creates list of fields for CKAN Dataset.
   */
  public function getCkanOrganizationFields() {
    return array(
      "title" => "Title",
      "created" => "Created",
      "description" => "Description",
      "revision_timestamp" => "Changed",
      "name" => "Path",
      "image_url" => "Image URL",
    );
  }
}
